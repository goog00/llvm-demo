#include <iostream>
#include <vector>

void compute(const std::vector<int>& a, const std::vector<int>& b, std::vector<std::vector<int>>& c) {
    int n = a.size();
    int m = b.size();
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++) {
            c[i][j] = a[i] * b[j];
        }
    }
}

int main() {
    int n = 1000;
    int m = 1000;

    std::vector<int> a(n, 1);
    std::vector<int> b(m, 2);
    std::vector<std::vector<int>> c(n, std::vector<int>(m, 0));

    compute(a, b, c);

    std::cout << "c[0][0] = " << c[0][0] << std::endl;
    std::cout << "c[n-1][m-1] = " << c[n-1][m-1] << std::endl;

    return 0;
}
