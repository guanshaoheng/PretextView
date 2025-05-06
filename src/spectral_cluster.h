
#ifndef SPECTRAL_CLUSTER_H
#define SPECTRAL_CLUSTER_H


#include <Eigen/Dense>
#include <vector>
#include <iostream>
#include <random>
#include <limits>
#include <cmath>
#include <ctime>
#include "self_matrix.h"


// Gaussian kernel
double gaussian_kernel(const Eigen::VectorXd& xi, const Eigen::VectorXd& xj, double sigma) {
    return exp(-(xi - xj).squaredNorm() / (2 * sigma * sigma));
}

// Similarity matrix
Eigen::MatrixXd buildSimilarityMatrix(const Eigen::MatrixXd& X, double sigma) {
    int n = X.rows();
    Eigen::MatrixXd W(n, n);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            W(i, j) = gaussian_kernel(X.row(i), X.row(j), sigma);
    return W;
}

// Degree matrix
Eigen::MatrixXd buildDegreeMatrix(const Eigen::MatrixXd& W) {
    int n = W.rows();
    Eigen::MatrixXd D = Eigen::MatrixXd::Zero(n, n);
    for (int i = 0; i < n; ++i)
        D(i, i) = W.row(i).sum();
    return D;
}

// Basic KMeans clustering
Eigen::VectorXi kmeans(const Eigen::MatrixXd& data, int k, int max_iters = 100) {
    int n = data.rows();
    int d = data.cols();
    Eigen::VectorXi labels(n);
    Eigen::MatrixXd centroids = Eigen::MatrixXd::Zero(k, d);
    
    // Randomly initialize centroids
    std::srand(static_cast<unsigned int>(time(nullptr)));
    for (int i = 0; i < k; ++i)
        centroids.row(i) = data.row(std::rand() % n);
    
    for (int iter = 0; iter < max_iters; ++iter) {
        // Step 1: Assign labels
        for (int i = 0; i < n; ++i) {
            double min_dist = std::numeric_limits<double>::max();
            int best_cluster = 0;
            for (int j = 0; j < k; ++j) {
                double dist = (data.row(i) - centroids.row(j)).squaredNorm();
                if (dist < min_dist) {
                    min_dist = dist;
                    best_cluster = j;
                }
            }
            labels[i] = best_cluster;
        }

        // Step 2: Update centroids
        Eigen::MatrixXd new_centroids = Eigen::MatrixXd::Zero(k, d);
        Eigen::VectorXi count = Eigen::VectorXi::Zero(k);
        for (int i = 0; i < n; ++i) {
            new_centroids.row(labels[i]) += data.row(i);
            count[labels[i]]++;
        }
        for (int j = 0; j < k; ++j)
            if (count[j] > 0)
                new_centroids.row(j) /= count[j];

        // Check convergence
        if ((centroids - new_centroids).norm() < 1e-4)
            break;
        centroids = new_centroids;
    }

    return labels;
}


Eigen::MatrixXd selfmatrix_to_eigenMatrix(const Matrix3D<float>& adj_mat) {
    Eigen::MatrixXd norm_mat(adj_mat.row_num, adj_mat.col_num);
    float max_tmp = 0;
    for (int i = 0; i < norm_mat.rows(); ++i) {
        for (int j = 0; j < norm_mat.cols(); ++j) {
            max_tmp = -1.;
            for (int k = 0; k < 4; ++k) {
                max_tmp = std::max(max_tmp, adj_mat(i, j, k));
            }
            norm_mat(i, j) = max_tmp;
        }
    }
    return norm_mat;
}


std::vector<std::vector<int>> labels_to_clusters(const Eigen::VectorXi& labels, int k) {
    std::vector<std::vector<int>> clusters(k);
    for (int i = 0; i < labels.size(); ++i) {
        clusters[labels[i]].push_back(i);
    }
    return clusters;
}


// Main spectral clustering function
std::vector<std::vector<int>> spectral_clustering(const Matrix3D<float>& adj_mat, int k) {

    // Step 1: Similarity matrix
    Eigen::MatrixXd W = selfmatrix_to_eigenMatrix(adj_mat);

    // Step 2: Degree matrix
    Eigen::MatrixXd D = buildDegreeMatrix(W);
    // Step 3: Laplacian
    Eigen::MatrixXd L = D - W;
    // Step 4: Eigen decomposition
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> eigensolver(L);
    if (eigensolver.info() != Eigen::Success)
        throw std::runtime_error("Eigen decomposition failed");

    // Step 5: Use first k smallest eigenvectors
    Eigen::MatrixXd eigvecs = eigensolver.eigenvectors().leftCols(k);

    // Optional: Row normalization
    for (int i = 0; i < eigvecs.rows(); ++i)
        eigvecs.row(i).normalize();

    // Step 6: KMeans on rows of eigvecs
    Eigen::VectorXi labels = kmeans(eigvecs, k);

    return labels_to_clusters(labels, k);
}

#endif // SPECTRAL_CLUSTER_H