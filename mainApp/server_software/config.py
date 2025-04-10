from threading import Lock

class RuntimeConfig:
    def __init__(self):
        self.lock = Lock()
        self.start = True
        self.chunk_size = 3
        self.language = "en-US"
        self.esp32_ip = None

    def set_ip(self, ip):
        with self.lock:
            self.esp32_ip = ip

    def get_ip(self):
        with self.lock:
            return self.esp32_ip

    def update(self, updates: dict):
        with self.lock:
            if "start" in updates:
                self.start = bool(int(updates["start"]))
            if "font" in updates:
                self.chunk_size = { "8": 4, "12": 3, "16": 2 }.get(updates["font"], 3)
            if "lang" in updates:
                self.language = {
                    "English": "en-US",
                    "Spanish": "es-ES",
                    "Hindi": "hi-IN",
                    "Mandarin": "zh-CN",
                    "Arabic": "ar-SA",
                    "Portugues": "pt-PT"
                }.get(updates["lang"], "es-ES")

    
    def snapshot(self):
        with self.lock:
            return self.start, self.chunk_size, self.language
