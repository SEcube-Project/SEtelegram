/**
  ******************************************************************************
  * File Name          : SEcubeServer.cpp
  * Description        : Interface for the communication between the SEtelegram GUI and the SEcube SDK
  ******************************************************************************
  *
  * Copyright ? 2016-present Blu5 Group <https://www.blu5group.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 3 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, see <https://www.gnu.org/licenses/>.
  *
  ******************************************************************************
  */

#include <iostream>
#include <arpa/inet.h>
#include <fstream>
#include "sekey/SEkey.h"
#include "setelegram/SEtelegram.h"

#define BUFLEN 1024*1024

std::string getEncOrDecPath(const std::string& path, const std::string& mode);

int main(int argc, char *argv[]) {
	int s0, s1;
	std::unique_ptr<L0> l0;
	std::unique_ptr<L1> l1;
	bool quit = false;
	std::ofstream logfile("/home/matteo/Scrivania/SEcubeServer_log.txt");

	logfile << "Server started" << std::endl;
	if (argc < 2) {
		logfile << "usage: SEcubeServer_path port_number GUI_thread_id" << std::endl;
		exit(1);
	}

	std::string arg1(argv[1]);
	int listenPort;
	try {
	  std::size_t pos;
	  listenPort = std::stoi(arg1, &pos);
	  if (pos < arg1.size()) {
	    std::cerr << "Trailing characters after number: " << arg1 << '\n';
	    exit(1);
	  }
	} catch (std::invalid_argument const &ex) {
	  std::cerr << "Invalid number: " << arg1 << '\n';
	  exit(1);
	} catch (std::out_of_range const &ex) {
	  std::cerr << "Number out of range: " << arg1 << '\n';
	  exit(1);
	}

	s0 = socket(AF_INET, SOCK_STREAM, 0);
	if (s0 < 0) {
		logfile << "Error creating socket: " << strerror(errno) << std::endl;
		exit(1);
	}
	struct sockaddr_in address;
	memset(&address, 0, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_port = htons(listenPort);
	address.sin_addr.s_addr = inet_addr("127.0.0.1");

	int res = bind(s0, (struct sockaddr*) &address, sizeof(address));
	if (res < 0) {
		logfile << "Error binding: " << strerror(errno) << std::endl;
		exit(1);
	}

	// Set the "LINGER" timeout to zero, to close the listen socket
	// immediately at program termination.
	struct linger linger_opt = { 1, 0 }; // Linger active, timeout 0
	setsockopt(s0, SOL_SOCKET, SO_LINGER, &linger_opt, sizeof(linger_opt));

	// Listen for a connection
	res = listen(s0, 1);    // "1" is the maximal length of the queue
	if (res < 0) {
		logfile << "Error listening: " << strerror(errno) << std::endl;
		exit(1);
	}

	struct sockaddr_in peer_address;
	socklen_t peer_address_len;
	logfile << "Waiting for connection" << std::endl;
	s1 = accept(s0, (struct sockaddr*) &peer_address, &peer_address_len);
	if (s1 < 0) {
		logfile << "Error accepting connection: " << strerror(errno) << std::endl;
		exit(1);
	}
	close(s0);

	while (!quit) {
		char GUI_request1[BUFLEN] = {0};
		res = read(s1, GUI_request1, BUFLEN-1);
		if (res < 0) {
			logfile << "Error reading: " << strerror(errno) << std::endl;
			send(s1, "2", 1, 0);
			continue;
		}
		GUI_request1[res] = 0;
		logfile << "SEcube server received " << res << " bytes: " << GUI_request1 << std::endl;

		if (strlen(GUI_request1) == 4 && (strcmp(GUI_request1, "quit") == 0)) {
			logfile << "SEcube server quit" << std::endl;
			break;
		}
		else if (strlen(GUI_request1) < 6 || (strncmp("login_", GUI_request1, 6) != 0)) {
			logfile << "Wrong request from client" << std::endl;
			send(s1, "3", 1, 0);
			continue;
		}

		l0 = std::make_unique<L0>();
		l1 = std::make_unique<L1>();
		uint8_t numDevices = l0->GetNumberDevices();
		if (numDevices == 0) {
			logfile << "SEcube not connected" << std::endl;
			send(s1, "err_nosec", 9, 0);
			continue;
		}
		else if (numDevices > 1) {
			logfile << "Too many SEcube connected" << std::endl;
			send(s1, "err_toomany", 11, 0);
			continue;
		}
		uint8_t* pin_user = (uint8_t*)GUI_request1+6;
		std::array<uint8_t, 32> pin = {0};
		for(uint16_t i=0; i<strlen((const char*)pin_user); i++){
			pin.at(i) = pin_user[i];
		}
		try {
			l1->L1Login(pin, SE3_ACCESS_USER, true);
		} catch(...) {
			logfile << "Error during login" << std::endl;
			send(s1, "err_login", 9, 0);
			continue;
		}
		logfile << "Login done" << std::endl;

		if(sekey_start(*l0, l1.get()) != 0){
			logfile << "Error starting SEkey" << std::endl;
			send(s1, "err_sekey", 9, 0);
			continue;
		}
		logfile << "SEkey started" << std::endl;

		setelegram telegram(*l0, *l1);
		if(telegram.isvalid()){
			logfile << "Telegram-SEkey association database correctly loaded" << std::endl;
			send(s1, "setup_ok", 8, 0);
		} else {
			sekey_stop();
			logfile << "Error loading Telegram-SEkey association database" << std::endl;
			send(s1, "err_db", 6, 0);
			continue;
		}

		while (!quit) {
			char GUI_request2[BUFLEN] = {0};
			res = read(s1, GUI_request2, BUFLEN-1);
			if (res < 0) {
				logfile << "Error reading: " << strerror(errno) << std::endl;
				send(s1, "2", 1, 0);
				continue;
			}
			GUI_request2[res] = 0;
			logfile << "Server received " << res << " bytes: " << GUI_request2 << std::endl;

			if (strlen(GUI_request2) > 15 && (strncmp("is_secube_chat_", GUI_request2, 15) == 0)) {
				std::string chatID(GUI_request2+15);
				int isTelegram = telegram.setelegram_find_chatID(chatID);
				if(isTelegram == 0){
					send(s1, "0", 1, 0); // found, not encrypted chat (client will set flag to 0)
				} else if(isTelegram == 1){
					send(s1, "1", 1, 0); // found, encrypted chat (client will set flag to 1)
				} else if(isTelegram == 3){
					send(s1, "2", 1, 0); // not found or any other error (client will set flag to -1)
				} else {
					send(s1, "3", 1, 0); // error
				}
			}

			// add entry to encrypted chat table
			else if (strlen(GUI_request2) > 14 && (strncmp("add_enc_table_", GUI_request2, 14) == 0)) {
				std::string delimiter = "_";
				std::string segment(GUI_request2+14);
				std::string chatID = segment.substr(0, segment.find(delimiter));
				std::string SEkeyID = segment.substr(segment.find(delimiter)+1, std::string::npos);
				int res = telegram.add_enc_table(chatID, SEkeyID);
				if(res == 0){
					send(s1, "0", 1, 0); // entry added correctly
				} else {
					send(s1, "1", 1, 0); // error
				}
			}

			// add entry to non-encrypted chat table
			else if (strlen(GUI_request2) > 18 && (strncmp("add_not_enc_table_", GUI_request2, 18) == 0)) {
				std::string delimiter = "_";
				std::string segment(GUI_request2+18);
				std::string chatID = segment.substr(0, segment.find(delimiter));
				int res = telegram.add_not_enc_table(chatID);
				if(res == 0){
					send(s1, "0", 1, 0); // entry added correctly
				} else {
					send(s1, "1", 1, 0); // error
				}
			}

			//encrypt_chatid_datatype_data
			else if (strlen(GUI_request2) > 8 && (strncmp("encrypt_", GUI_request2, 8) == 0)) {
				std::string info(GUI_request2+8);
				auto pos_1 = info.find('_');
				if (pos_1 == std::string::npos) {
					logfile << "Error in received encryption request" << std::endl;
					send(s1, "3", 1, 0);
					continue;
				}
				std::string chat_id = info.substr(0, pos_1);
				auto pos_2 = info.find('_', pos_1+1);
				if (pos_2 == std::string::npos) {
					logfile << "Error in received encryption request" << std::endl;
					send(s1, "3", 1, 0);
					continue;
				}
				std::string data_type = info.substr(pos_1+1,pos_2-(pos_1+1));
				std::string data = info.substr(pos_2+1, info.length());
				std::string enc_result;

				std::vector<unsigned char> plaintext;
				if (data_type.compare("file") == 0) {
					std::ifstream file(data, std::ios::binary);
					plaintext.assign(std::istreambuf_iterator<char>(file), {});
					file.close();
				}
				else if (data_type.compare("text") == 0) {
					plaintext.assign(data.begin(), data.end());
				}
				else {
					logfile << "Error in received encryption request" << std::endl;
					send(s1, "3", 1, 0);
					continue;
				}

				size_t plaintext_size = plaintext.size();
				std::shared_ptr<uint8_t[]> databuf(new uint8_t[plaintext_size]);
				memcpy(databuf.get(), plaintext.data(), plaintext_size);
				SElink ciphertext;
				std::string recipient = chat_id;
				if(telegram.setelegram_encrypt(l1.get(), databuf, plaintext_size, ciphertext, recipient) != 0){
					logfile << "Error in encryption" << std::endl;
					send(s1, "1", 1, 0);
					continue;
				}

				size_t serialized_size = 0;
				// serialize the encrypted data
				std::unique_ptr<char[]> serialized;
				ciphertext.serialize(serialized, serialized_size);
				if (data_type.compare("text") == 0) {
					enc_result = serialized.get();
				}
				else {
					auto enc_file_path = getEncOrDecPath(data, "enc");
					std::ofstream enc_file(enc_file_path, std::ios::binary);
					// here we write the serialized data to a file
					enc_file.write((const char *) serialized.get(), serialized_size);
					enc_file.close();
					enc_result = enc_file_path;
				}
				std::string server_resp = "0_" + enc_result;
				send(s1, server_resp.c_str(), server_resp.length(), 0);
			}

			//decrypt_datatype_data
			else if (strlen(GUI_request2) > 8 && (strncmp("decrypt_", GUI_request2, 8) == 0)) {
				std::string info(GUI_request2+8);
				auto pos = info.find('_');
				if (pos == std::string::npos) {
					logfile << "Error in received decryption request" << std::endl;
					send(s1, "3", 1, 0);
					continue;
				}
				std::string data_type = info.substr(0,pos);
				std::string data = info.substr(pos+1,info.length());

				SElink deserialized;

				if (data_type.compare("file") == 0) {
					std::ifstream enc_file(data, std::ios::binary);
					std::vector<char> serialized_file(std::istreambuf_iterator<char>(enc_file), {});
					enc_file.close();
					std::unique_ptr<char[]> buffer_b64 = std::make_unique<char[]>(serialized_file.size());
					memcpy(buffer_b64.get(), serialized_file.data(), serialized_file.size());
					deserialized.deserialize(buffer_b64, serialized_file.size());
				}
				else if (data_type.compare("text") == 0) {
					std::vector<char> enc_text(data.begin(), data.end());
					std::unique_ptr<char[]> buffer_b64 = std::make_unique<char[]>(enc_text.size());
					memcpy(buffer_b64.get(), enc_text.data(), enc_text.size());
					deserialized.deserialize(buffer_b64, enc_text.size());
				}
				else {
					logfile << "Error in received decryption request" << std::endl;
					send(s1, "3", 1, 0);
					continue;
				}

				size_t decrypted_size;
				std::shared_ptr<uint8_t[]> decrypted;
				if(telegram.setelegram_decrypt(l1.get(), decrypted, decrypted_size, deserialized) != 0){
					logfile << "Error decrypting data" << std::endl;
					send(s1, "1", 1, 0);
					continue;
				}
				std::string dec_result;
				if(data_type.compare("text") == 0) {
					std::string dec_text(decrypted.get(), decrypted.get() + decrypted_size);
					dec_result = dec_text;
				}
				else {
					auto file_path = getEncOrDecPath(data, "dec");
					// here we write the decrypted data to a file
					std::ofstream file(file_path, std::ios::binary);
					file.write((const char *) decrypted.get(), decrypted_size);
					file.close();
					dec_result = file_path;
				}
				std::string server_resp = "0_" + dec_result;
				send(s1, server_resp.c_str(), server_resp.length(), 0);
			}

			else if (strlen(GUI_request2) == 4 && (strcmp(GUI_request2, "quit") == 0)) {
				quit = true;
				sekey_stop();
				logfile << "SEkey stopped" << std::endl;
				l1->L1Logout();
				logfile << "Logout" << std::endl;
			}

			else {
				logfile << "Wrong request from client" << std::endl;
				send(s1, "3", 1, 0);
			}
		}
	}

	close(s1);
    return 0;
}

std::string getEncOrDecPath(const std::string& path, const std::string& mode) {
    std::string final_path;
    if (mode == "enc") {
    	final_path = path + ".enc";
    }
    else if (mode == "dec") {
    	final_path = path.substr(0, path.length()-4);
    }
    return final_path;
}
