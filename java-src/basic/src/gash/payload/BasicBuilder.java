package gash.payload;

/**
 * the builder to construct commands that both the client (BasicSocketClient)
 * and server (BasicServer) understands/accepts
 * 
 * @author gash
 * 
 */
public class BasicBuilder {

	public BasicBuilder() {
	}

	public String encode(Message msg) {
        var payload = msg.getGroup() + "," + msg.getName() + "," + msg.getText();
		return String.format("%04d", payload.length())  + "," + payload;
	}

	public Message decode(byte[] raw) throws Exception {
		if (raw == null || raw.length == 0)
			return null;

		var s = new String(raw);
		var parts = s.split(",", 4);
		var rtn = new Message(parts[2], parts[1], parts[3]);

		return rtn;
	}
}
