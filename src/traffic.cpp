#include <bits/stdc++.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

// ---- 五元组与哈希 ----
struct FiveTuple {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    bool operator==(const FiveTuple& o) const noexcept {
        return src_ip==o.src_ip && dst_ip==o.dst_ip && src_port==o.src_port && dst_port==o.dst_port;
    }
};
struct FiveTupleHash {
    size_t operator()(const FiveTuple& t) const noexcept {
        uint64_t k = (uint64_t)t.src_ip<<32 | t.dst_ip;
        k ^= (uint64_t)t.src_port<<16;
        k ^= (uint64_t)t.dst_port;
        // 64->size_t 混合
        k ^= (k>>33); k *= 0xff51afd7ed558ccdULL;
        k ^= (k>>33); k *= 0xc4ceb9fe1a85ec53ULL;
        k ^= (k>>33);
        return (size_t)k;
    }
};

// ---- TCP 流状态：syn_count + seen_other ----
struct FlowState {
    uint8_t syn_count = 0;
    bool seen_other = false;
};

// ---- IP 字符串<->整数 ----
static inline uint32_t ip2int(const char* s, size_t len) {
    // s like "A.B.C.D"
    uint32_t parts[4] = {0,0,0,0};
    int idx=0;
    uint32_t v = 0;
    for(size_t i=0;i<len;i++){
        char c = s[i];
        if(c=='.'){
            parts[idx++] = v;
            v=0;
            if(idx>3) break;
        }else{
            v = v*10 + (uint32_t)(c - '0');
        }
    }
    if(idx<=3) parts[idx]=v;
    return (parts[0]<<24)|(parts[1]<<16)|(parts[2]<<8)|parts[3];
}
static inline string int2ip(uint32_t ip) {
    return to_string((ip>>24)&255) + "." + to_string((ip>>16)&255) + "." +
           to_string((ip>>8)&255) + "." + to_string(ip&255);
}

// ---- 线程结果 ----
struct ThreadResult {
    unordered_map<FiveTuple, FlowState, FiveTupleHash> tcp; // 五元组 -> 状态
    unordered_map<uint32_t, uint64_t> dns;                  // 源 IP -> 累加前缀长度
    ThreadResult() {
        tcp.reserve(1<<20);
        dns.reserve(1<<18);
    }
};

// ---- 快速按空格切 token：返回每段的指针+长度 ----
static inline int split8(const char* line, size_t n, pair<const char*, size_t> out[8]) {
    // 最多切成 8 段（我们需要前 8 段）
    int cnt = 0;
    size_t i=0;
    while (i<n && cnt<8) {
        while (i<n && line[i]==' ') ++i;            // skip spaces
        if (i>=n) break;
    // 输出：先 portscan，再 tunnelling；各自 IP 按“字典序”（字符串比较）排序
    vector<pair<uint32_t,uint64_t>> v1(portscan_ip.begin(), portscan_ip.end());
    sort(v1.begin(), v1.end(), [](auto &a, auto &b){
        // 字符串字典序比较，满足 1.1.1.7 < 1.1.1.70 < 1.1.1.8
        return int2ip(a.first) < int2ip(b.first);
    });
    for (auto &kv : v1) {
        // 严格格式：IP 空格 portscan 空格 次数
        cout << int2ip(kv.first) << " portscan " << kv.second << '\n';
    }

    vector<pair<uint32_t,uint64_t>> v2(dns_all.begin(), dns_all.end());
    sort(v2.begin(), v2.end(), [](auto &a, auto &b){
        return int2ip(a.first) < int2ip(b.first);
    });
    for (auto &kv : v2) {
        cout << int2ip(kv.first) << " tunnelling " << kv.second << '\n';
    }

    if (is_regular) {
        munmap(mm, sz);
        close(fd);
    }
    return 0;
}