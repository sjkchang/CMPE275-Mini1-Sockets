package gash.socket;

import java.io.BufferedInputStream;
import java.io.InterruptedIOException;
import java.net.Socket;
import java.util.ArrayList;

import gash.payload.BasicBuilder;
import gash.payload.Message;


/**
 * 
 * @author gash
 * 
 */
class SessionHandler extends Thread {
	private Socket connection;
	private String name;
	private boolean forever = true;
	

	public SessionHandler(Socket connection) {
		this.connection = connection;
		
		// allow server to exit if
		this.setDaemon(true);
	}

	/**
	 * stops session on next timeout cycle
	 */
	public void stopSession() {
		forever = false;
		if (connection != null) {
			try {
				connection.close();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		connection = null;
	}

	/**
	 * process incoming data
	 */
	public void run() {
		System.out.println("Session " + this.getId() + " started");

		try {
			connection.setSoTimeout(2000);
			var in = new BufferedInputStream(connection.getInputStream());

			if (in == null)
				throw new RuntimeException("Unable to get input streams");

			byte[] raw = new byte[2048];
			BasicBuilder builder = new BasicBuilder();
            ArrayList<Byte> overflow_buffer = new ArrayList<>();
			while (forever) {
				try {
					int bytesRead = in.read(raw);
					if (bytesRead == 0){
						continue;
                    } else if (bytesRead == -1){
						break;
                    }

                    if(overflow_buffer.size() > 0){
                        byte[] temp = new byte[overflow_buffer.size() + bytesRead];
                        for(int i = 0; i < overflow_buffer.size(); i++){
                            temp[i] = overflow_buffer.get(i);
                        }
                        for(int i = 0; i < bytesRead; i++){
                            temp[i + overflow_buffer.size()] = raw[i];
                        }
                        raw = temp;
                        bytesRead = raw.length;
                        overflow_buffer.clear();
                    }

                    int pos = 0;
                    while(pos < bytesRead){
                        // Check if we have enough bytes to read the length of the message
                        if(bytesRead - pos < 5){
                            for(int i = pos; i < bytesRead; i++){
                                overflow_buffer.add(raw[i]);
                            }
                            break;
                        }

                        // Read the length of the message
                        int messageLength = Integer.parseInt(new String(raw, pos, 4));
                        System.out.println("Message Length: " + messageLength);
                        if(messageLength + (pos + 5) > bytesRead){
                            for(int i = pos; i < bytesRead; i++){
                                overflow_buffer.add(raw[i]);
                            }
                            break;
                        }

                        Message msg = builder.decode(new String(raw, pos, messageLength + 5).getBytes());
                        System.out.println("[L]: " + msg.toString().length() +"[T]: " + msg.getClass().getName() + " [M]: " + msg );
                        pos += messageLength + 5;
                    }

                
				} catch (InterruptedIOException ioe) {
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				System.out.println("Session " + this.getId() + " ending");
				System.out.flush();
				stopSession();
			} catch (Exception re) {
				re.printStackTrace();
			}
		}
	}

} // class SessionHandler