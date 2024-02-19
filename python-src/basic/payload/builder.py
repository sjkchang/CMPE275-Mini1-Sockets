
class BasicBuilder(object):
    def __init__(self):
        pass
        
    def encode(self, name, group, msg):
        # TODO encode message
        payload = (f"{group},{name},{msg}")
        return (f"{len(payload):04d},{payload}")

    def decode(self, message):
        # TODO complete parsing
        parts = message.split(",", 3)
        if len(parts) is not 3:
            raise ValueError(f"message format error: {message}")
        else:
            return parts[1],parts[0],parts[2]
