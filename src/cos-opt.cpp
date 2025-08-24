#include <bits/stdc++.h>
#include <faiss/IndexFlat.h>
#include <faiss/utils/Heap.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // === 读取输入 ===
    uint32_t N, D;
    fread(&N, sizeof(uint32_t), 1, stdin);
    fread(&D, sizeof(uint32_t), 1, stdin);

    vector<float> data(N * D);
    fread(data.data(), sizeof(float), N * D, stdin);

    // === 归一化（就地 L2 norm） ===
    for (size_t i = 0; i < N; i++) {
        float norm = 0;
        float* v = data.data() + i * D;
        for (size_t j = 0; j < D; j++) norm += v[j] * v[j];
        norm = 1.0f / (sqrtf(norm) + 1e-12f);
        for (size_t j = 0; j < D; j++) v[j] *= norm;
    }

    // === 构建 FAISS IndexFlatIP ===
    faiss::IndexFlatIP index(D);
    index.add(N, data.data());

    // === 搜索 top-5（包含自己） ===
    vector<float> sims(N * 5);
    vector<faiss::idx_t> idxs(N * 5);
    index.search(N, data.data(), 5, sims.data(), idxs.data());

    // === 去掉自己，保留 top-4 ===
    vector<float> result(N * 4);
    for (size_t i = 0; i < N; i++) {
        // sims[i*5 + 0] 是自己，跳过
        memcpy(result.data() + i * 4, sims.data() + i * 5 + 1, 4 * sizeof(float));
    }

    // === 写输出（小端 float32） ===
    fwrite(result.data(), sizeof(float), result.size(), stdout);
    return 0;
}