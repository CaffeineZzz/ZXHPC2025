import sys
import json
import urllib.request
import urllib.error
from concurrent.futures import ThreadPoolExecutor, as_completed

LLAMA_SERVER_URL = "http://127.0.0.1:11411/v1/completions"
LOG_FILE = "llama_batch_log.jsonl"
MAX_WORKERS = 28  # 并发线程数

PROMPT_TEMPLATE = """{q}.If unsure, choose B.Answer:"""

def read_questions():
    content = sys.stdin.read()
    questions = [q.strip() for q in content.split("\n\n") if q.strip()]
    return questions

def build_payload(question):
    prompt = PROMPT_TEMPLATE.format(q=question)
    payload = json.dumps({
        "prompt": prompt,
        "max_tokens": 1,
        "temperature": 1.0,    # 采样温度，0.0=贪心，越大越随机
        "top_p": 0.6,          # nucleus sampling，0.0-1.0
        "top_k": 4,
        "min_p": 0,
        "presence_penalty": 0
    }).encode("utf-8")
    return payload

def fetch_answer(idx, question):
    payload = build_payload(question)
    req = urllib.request.Request(LLAMA_SERVER_URL, data=payload, headers={"Content-Type": "application/
json"})
    try:
        with urllib.request.urlopen(req, timeout=60) as resp:
            data = json.load(resp)
            # 写日志
            # log_entry = {"index": idx, "question": question, "response": data}
            # with open(LOG_FILE, "a", encoding="utf-8") as f:
            #     f.write(json.dumps(log_entry, ensure_ascii=False) + "\n")
            # 提取答案
            text = data.get("choices", [{}])[0].get("text", "")
            ans = "B"
            for c in text.upper():
                if c in "ABCD":
                    ans = c
                    break
            return idx, ans
    except Exception as e:
        # log_entry = {"index": idx, "question": question, "error": str(e)}
        # with open(LOG_FILE, "a", encoding="utf-8") as f:
        #     f.write(json.dumps(log_entry, ensure_ascii=False) + "\n")
        return idx, "B"

def main():
    questions = read_questions()
    results = [None] * len(questions)
    with ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        futures = {executor.submit(fetch_answer, idx, q): idx for idx, q in enumerate(questions)}
        for future in as_completed(futures):
            idx, ans = future.result()
            results[idx] = ans
    # 输出答案
    for ans in results:
        print(ans)

if __name__ == "__main__":
    main()