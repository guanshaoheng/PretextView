import torch
import torch.nn as nn

class SimpleModel(nn.Module):
    def __init__(self):
        super(SimpleModel, self).__init__()
        self.fc = nn.Linear(10, 5)

    def forward(self, x):
        return self.fc(x)


model = SimpleModel()

example_input = torch.randn(1, 10)
scripted_model = torch.jit.trace(model, example_input)
scripted_model.save("model_scripted.pt")