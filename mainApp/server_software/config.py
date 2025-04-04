from threading import Lock

class RuntimeConfig:
    def __init__(self):
        self.lock = Lock()
        self.start = True
        self.chunk_size = 2
        self.language = "es-ES"

    def update(self, updates: dict):
        with self.lock:
            if "start" in updates:
                self.start = bool(int(updates["start"]))
            if "size" in updates:
                self.chunk_size = { "0": 3, "1": 2, "2": 1 }.get(updates["size"], 2)
            if "lang" in updates:
                self.language = {
                    "0": "en-US",
                    "1": "es-ES",
                    "2": "hi-IN",
                    "3": "zh-CN",
                    "4": "ar-SA",
                    "5": "pt-PT"
                }.get(updates["lang"], "es-ES")

    
    def snapshot(self):
        with self.lock:
            return self.start, self.chunk_size, self.language
