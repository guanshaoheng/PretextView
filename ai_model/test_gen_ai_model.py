import torch
import torch.nn as nn
from torch_geometric.nn import GCNConv, GATConv


class Graph_HiC_Likelihood(nn.Module):
    def __init__(self, input_dim=8, hidden_dim=128, hidden_likelihood_dim=128, heads_num=4, num_edge_attr=7, output_dim=4):
        super().__init__()

        self.num_edge_attr = num_edge_attr
        
        self.edge_fc_layers = torch.nn.ModuleList([
            torch.nn.ModuleList([
                nn.Linear(num_edge_attr, num_edge_attr * 4 ), 
                nn.Linear(num_edge_attr * 4, num_edge_attr * 4),
                nn.Linear(num_edge_attr * 4, 1)
            ]) for _ in range(4)
        ])

        # self.conv_attention = GATConv(input_dim, hidden_dim, heads=heads_num)
        self.conv_list_0 = torch.nn.ModuleList([GCNConv(input_dim, hidden_dim) for _ in range(self.num_edge_attr)])
        self.conv_list_1 = GCNConv(hidden_dim * self.num_edge_attr, hidden_dim)
        self.conv_list_2 = torch.nn.ModuleList([GCNConv(hidden_dim, hidden_dim) for _ in range(self.num_edge_attr)])
        self.conv_list_3 = torch.nn.ModuleList([GCNConv(hidden_dim*self.num_edge_attr, hidden_dim) for _ in range(self.num_edge_attr)])
        
        self.link_likelihood_hidden_list = torch.nn.ModuleList(
            [nn.Linear(hidden_dim * 2*self.num_edge_attr, hidden_likelihood_dim)] +\
            [nn.Linear(hidden_likelihood_dim, hidden_likelihood_dim) for _ in range(3)] + \
            [nn.Linear(hidden_likelihood_dim, output_dim)]
        )

        self.sigmoid = torch.nn.Sigmoid()
        self.relu = torch.nn.ReLU()
        self.dropout = torch.nn.Dropout(p=0.5)
    
    def multi_edge_to_one(self, edge_attr:torch.tensor, i:int):
        tmp = self.relu(self.edge_fc_layers[i][0](edge_attr))
        for layer in self.edge_fc_layers[i][1:-1]:
            tmp = self.relu(layer(tmp))
        return self.sigmoid(self.edge_fc_layers[i][-1](tmp).squeeze(-1))

    def forward_batch(self, batch):
        x, edge_index, edge_attr, edge_index_test = batch.x, batch.edge_index, batch.edge_attr, batch.edge_index_test

        x = self.forward(x, edge_index, edge_attr, edge_index_test)
        return x  #  likelihood of the edges
    
    def forward(self, x:torch.Tensor, edge_index:torch.Tensor, edge_attr:torch.Tensor, edge_index_test:torch.Tensor):
        '''
            The forward function for the model to be called in the C++ code
        '''
        x = self.forward_x_embedding(x, edge_index, edge_attr)
        x = self.dropout(x) # [node_num, hidden_dim * self.num_edge_attr]
        x = self.predict_links(x, edge_index_test)  # torch.any(x.isnan())
        return x  #  likelihood of the edges
    
    def predict_links(self, x, edge_index):
        """
            FINISHED there maybe some problem as the prediction of  [node_1, node_2] should be 
                different from the prediction of [node_2, node_1]

                [node_1, node_2] : [head1->head1, head1->tail2, tail1->head2, tail1->tail2]
                [node_2, node_1] : [head2->head1, head2->tail1, tail2->head1, tail2->tail1]

                            0 - 1
                            |   |
                            2 - 3

                0 - 2
                |   |
                1 - 3
        """
        edge_embeddings = torch.cat([x[edge_index[0]], x[edge_index[1]]], dim=-1)
        edge_embeddings_1 = torch.cat([x[edge_index[1]], x[edge_index[0]]], dim=-1)
        tmp1 = self.cal_likelihood_full_connencted_layers(edge_embeddings)
        tmp2 = self.cal_likelihood_full_connencted_layers(edge_embeddings_1)[:, [0, 2, 1, 3]]
        return 0.5 * (tmp1 + tmp2)
    
    def cal_likelihood_full_connencted_layers(self, x):
        for layer in self.link_likelihood_hidden_list[:-1]:
            x = self.relu(layer(x))
        return self.link_likelihood_hidden_list[-1](x).squeeze(-1)
    
    def forward_x_embedding(self, x:torch.Tensor, edge_index:torch.Tensor, edge_attr:torch.Tensor):

        # four parallell layers of the graph convolution
        # x = self.relu(
        #     self.conv_attention(x, edge_index)   # [64, 64 * heads_num]   
        # )
        x = self.relu(
            torch.concat([self.conv_list_0[i](x, edge_index, edge_weight=edge_attr[:, i]) for i in range(self.num_edge_attr)], dim=-1)  # [64, 64 * self.num_edge_attr]
        )
        x = self.relu(
            self.conv_list_1(x, edge_index, edge_weight=self.multi_edge_to_one(edge_attr, 1))   # [64, 64 * self.num_edge_attr]
        )
        x = self.relu(
            torch.concat([self.conv_list_2[i](x, edge_index, edge_weight=edge_attr[:, i]) for i in range(self.num_edge_attr)], dim=-1)   # [64, 64 * self.num_edge_attr]
        )
        x = self.relu(
            torch.concat([self.conv_list_3[i](x, edge_index, edge_weight=edge_attr[:, i]) for i in range(self.num_edge_attr)], dim=-1)   # [64, 64 * self.num_edge_attr]
        )
        return x  #  node embedding
    

if __name__ == "__main__":
    # Initialize the model and load its state_dict
    model = Graph_HiC_Likelihood()
    checkpoint = torch.load("ai_model/checkpoint_20241129-19:29.pth", map_location=torch.device('cpu'))
    model.load_state_dict(checkpoint['model_state_dict'])
    model.eval()  # Set the model to evaluation mode

    # Example inputs
    x = torch.randn(64, 8)  # Example node features (64 nodes, 8 features per node)
    edge_index = torch.tensor([[0, 1], [1, 0]], dtype=torch.long)  # Example edge index
    edge_attr = torch.randn(2, 7)  # Example edge attributes
    edge_index_test = torch.tensor([[0, 1], [1, 0]], dtype=torch.long)  # Example test edge index

    # Trace the model
    scripted_model = torch.jit.trace(model, (x, edge_index, edge_attr, edge_index_test))

    # Save the traced model
    scripted_model.save("ai_model/model_scripted.pt")
