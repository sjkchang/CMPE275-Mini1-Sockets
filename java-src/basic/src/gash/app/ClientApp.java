package gash.app;

import java.io.BufferedReader;
import java.io.InputStreamReader;

import gash.socket.BasicClient;

/**
 * client - basic chat construct. This varies from our Python and C++ versions
 * as it prompts the use for messages.
 * 
 * @author gash
 * 
 */
public class ClientApp {
	private BasicClient myClient;

	public ClientApp() {
	}

	public static void main(String[] args) {
		var myClient = new BasicClient("app", "127.0.0.1", 2000);
		myClient.connect();
		myClient.join("petsadogs");

		var br = new BufferedReader(new InputStreamReader(System.in));
		while (true) {
			try {
				System.out.print("\nenter how many messages to send ('exit' to quit): ");
				String m = br.readLine();
				if (m.length() == 0 || "exit".equalsIgnoreCase(m))
					break;

                int numMessages = Integer.parseInt(m);
				for (int i = 0; i < numMessages; i++) {
                    try {
                        myClient.sendMessage("This is message # " + String.valueOf(i));
                        System.out.println("sent message " + i);
                    } catch (Exception ex) {
                        break;
                    }
                }
			} catch (Exception ex) {
				break;
			}
		}
		
	}
}