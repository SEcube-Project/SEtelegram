/**
  ******************************************************************************
  * File Name          : SEtelegram.cpp
  * Description        : Specific SEcube SDK APIs related to the SEtelegram demo.
  ******************************************************************************
  *
  * Copyright © 2016-present Blu5 Group <https://www.blu5group.com>
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

#include "../sefile/environment.h"
#include "../sekey/SEkey.h"
#include "SEtelegram.h"

using namespace std;

setelegram::setelegram(){
	valid = false;
	setelegram_db = nullptr;
}

setelegram::setelegram(L0& l0, L1& l1){
	setelegram_db = nullptr;
	valid = false;
	try{
		// retrieve the path of the SEcube MicroSD
		string microsd, dbabsolutepath, query;
		if(get_microsd_path(l0, microsd)){
			return;
		}
		// create the SEfile object required to work with the DB
		unique_ptr<SEfile> telegramdb = make_unique<SEfile>();
		if(telegramdb->secure_init(&l1, L1Key::Id::RESERVED_ID_SETELEGRAM, L1Algorithms::Algorithms::AES_HMACSHA256)){
			return;
		}
		dbabsolutepath = microsd + "setelegram.sqlite";
		char dbname[MAX_PATHNAME];
		memset(dbname, '\0', MAX_PATHNAME);
		get_filename((char*)dbabsolutepath.c_str(), dbname);
		memcpy(telegramdb->handleptr->name, dbname, strlen(dbname));
		databases.push_back(std::move(telegramdb));
		// check if the database file already exists
		int rc, open_flags = 0;
		if((rc=file_exists(dbabsolutepath)) == SEKEY_FILE_NOT_FOUND){
			open_flags = SQLITE_OPEN_CREATE;
		} else {
			open_flags = 0; // bitwise OR operator with this value won't change SQLITE_OPEN_READWRITE
		}
		if(rc == SEKEY_ERR){
			return;
		}
		// open the database file
		statement sqlstmt;
		if(((rc = sqlite3_open_v2(dbabsolutepath.c_str(), &setelegram_db, SQLITE_OPEN_READWRITE | open_flags, nullptr)) != SQLITE_OK) ||
		   (sqlite3_exec(setelegram_db, "PRAGMA synchronous = 2;", nullptr, nullptr, nullptr) != SQLITE_OK)){
			return;
		}
		sqlite3_extended_result_codes(setelegram_db, 1);
		sqlite3_db_config(setelegram_db, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 0, nullptr);
		sqlite3_exec(setelegram_db, "PRAGMA journal_mode = DELETE;", nullptr, nullptr, nullptr);
		if(open_flags == SQLITE_OPEN_CREATE){
			query.assign("CREATE TABLE EncryptedChat(chat_id TEXT PRIMARY KEY, sekey_id TEXT NOT NULL);"
						 "CREATE TABLE ClearChat(chat_id TEXT PRIMARY KEY);");
			if((rc = sqlite3_exec(setelegram_db, query.c_str(), nullptr, nullptr, nullptr)) != SQLITE_OK){
				return;
			}
		}
		// in case of pending journal file on restart, restore the database using a "dummy" transaction
		if((sqlite3_exec(setelegram_db, "BEGIN;", nullptr, nullptr, nullptr) != SQLITE_OK) ||
		   (sqlite3_exec(setelegram_db, "CREATE TABLE mytable(myval INTEGER DEFAULT 0);", nullptr, nullptr, nullptr) != SQLITE_OK) ||
		   (sqlite3_exec(setelegram_db, "ROLLBACK;", nullptr, nullptr, nullptr) != SQLITE_OK)){
			return;
		}
		// check database integrity
		rc = sqlite3_prepare_v2(setelegram_db, "PRAGMA integrity_check", -1, sqlstmt.getstmtref(), nullptr);
		for(;;){
			if ((rc = sqlite3_step(sqlstmt.getstmt())) == SQLITE_DONE){
				break;
			}
			if (rc != SQLITE_ROW) {
				return;
			}
			string msg;
			msg.assign(sqlite3_column_text_wrapper(sqlstmt.getstmt(), 0));
			if(msg.compare("ok") != 0){
				return;
			}
		}
	} catch (...) {
		return;
	}
	valid = true; // set valid to true only if everything is ok
}

setelegram::~setelegram(){
	sqlite3_close(setelegram_db);
	valid = false;
}

bool setelegram::isvalid(){
	return this->valid;
}

/* Check if a Telegram chatID is stored into the database.
 * Return 0 if chatID is found and chat does not have to be encrypted.
 * Return 1 if chatID is found and chat must be encrypted.
 * Return 2 in case of any error.
 * Return 3 if chatID is not found. */
int setelegram::setelegram_find_chatID(string& chatID) {
	statement sqlstmt;
	int rc, count = 0;
	// check in encrypted chats
	string query = "SELECT COUNT(*) FROM EncryptedChat WHERE chat_id = ?1;";
	if((sqlite3_prepare_v2(this->setelegram_db, query.c_str(), -1, sqlstmt.getstmtref(), nullptr)!=SQLITE_OK) ||
	   (sqlite3_bind_text(sqlstmt.getstmt(), 1, chatID.c_str(), -1, SQLITE_STATIC)!=SQLITE_OK)){
		return 2;
	}
	for(;;){
		if ((rc = sqlite3_step(sqlstmt.getstmt())) == SQLITE_DONE){
			break;
		}
		if (rc != SQLITE_ROW) {
			return 2;
		}
		count = sqlite3_column_int64(sqlstmt.getstmt(), 0);
	}
	if(count > 0){ // if chatID is found in encrypted chats, return 1 immediately
		return 1;
	}
	// count = 0, not found...so check in not encrypted chats
	count = 0;
	query = "SELECT COUNT(*) FROM ClearChat WHERE chat_id = ?1;";
	if((sqlite3_prepare_v2(this->setelegram_db, query.c_str(), -1, sqlstmt.getstmtref(), nullptr)!=SQLITE_OK) ||
	   (sqlite3_bind_text(sqlstmt.getstmt(), 1, chatID.c_str(), -1, SQLITE_STATIC)!=SQLITE_OK)){
		return 2;
	}
	for(;;){
		if ((rc = sqlite3_step(sqlstmt.getstmt())) == SQLITE_DONE){
			break;
		}
		if (rc != SQLITE_ROW) {
			return 2;
		}
		count = sqlite3_column_int64(sqlstmt.getstmt(), 0);
	}
	if(count > 0){ // chat does not require encryption
		return 0;
	} else { // chat not found
		return 3;
	}
}

/* retrieves the group associated to a chat ID, then finds the most secure key for the group, finally encrypts the plaintext.
   returns 0 on success, -1 on error. */
int setelegram::setelegram_encrypt(L1 *l1, std::shared_ptr<uint8_t[]> plaintext,  size_t plaintext_size, SElink& ciphertext, string& chatID){
	if((l1 == nullptr) || (plaintext == nullptr) || (plaintext_size <= 0) || !this->valid){
		return -1;
	}
	try{
		statement sqlstmt;
		int rc;
		string query = "SELECT sekey_id FROM EncryptedChat WHERE chat_id = ?1;", SEkeyID = "", chosen_key;
		if((sqlite3_prepare_v2(this->setelegram_db, query.c_str(), -1, sqlstmt.getstmtref(), nullptr)!=SQLITE_OK) ||
		   (sqlite3_bind_text(sqlstmt.getstmt(), 1, chatID.c_str(), -1, SQLITE_STATIC)!=SQLITE_OK)){
			return -1;
		}
		for(;;){
			if ((rc = sqlite3_step(sqlstmt.getstmt())) == SQLITE_DONE){
				break;
			}
			if (rc != SQLITE_ROW) {
				return -1;
			}
			SEkeyID = sqlite3_column_text_wrapper(sqlstmt.getstmt(), 0);
		}
		if(SEkeyID.empty()){
			return -1;
		}
		if(SEkeyID.at(0) == 'U'){ // user
			if(sekey_find_key_v1(chosen_key, SEkeyID, se_key_type::symmetric_data_encryption) != SEKEY_OK){
				return -1;
			}
		}
		if(SEkeyID.at(0) == 'G'){ // group
			if(sekey_find_key_v2(chosen_key, SEkeyID, se_key_type::symmetric_data_encryption) != SEKEY_OK){
				return -1;
			}
		}
		// retrieve number part of key ID (i.e. K123 -> 123)
		string ksub = chosen_key.substr(1);
		uint32_t keyID = stoul_wrap(ksub);
		// call manual API using retrieved key
		if(selink_encrypt_manual(l1, plaintext, plaintext_size, ciphertext, keyID) != SELINK_OK){
			return -1;
		}
		return 0;
	} catch (...) {
		return -1;
	}
}

// just a wrapper around selink_decrypt
int setelegram::setelegram_decrypt(L1 *l1, std::shared_ptr<uint8_t[]>& plaintext, size_t& plaintext_size, SElink& ciphertext){
	if(selink_decrypt(l1, plaintext, plaintext_size, ciphertext) != SELINK_OK){
		return -1;
	} else {
		return 0;
	}
}

/* add entry to the table of encrypted telegram chat. sekeyid can be the id of a user or a group, it must be compliant with the
 * id rules of sekey. this function does not check if the id is actually referring to an id that is stored in sekey (because
 * that is time consuming since it requires to access to another database). return 0 on success, -1 otherwise. */
int setelegram::add_enc_table(string& chatID, string& SEkeyID){
	try{
		statement sqlstmt;
		int rc = 0;
		if(chatID.empty() || SEkeyID.empty() || (!check_input(SEkeyID, 1) && !check_input(SEkeyID, 0)) || !this->valid){
			return -1;
		}
		string query = "INSERT INTO EncryptedChat(chat_id, sekey_id) VALUES(?1, ?2);";
		if((sqlite3_prepare_v2(this->setelegram_db, query.c_str(), -1, sqlstmt.getstmtref(), nullptr) != SQLITE_OK) ||
		   (sqlite3_bind_text(sqlstmt.getstmt(), 1, chatID.c_str(), -1, SQLITE_STATIC) != SQLITE_OK)	||
		   (sqlite3_bind_text(sqlstmt.getstmt(), 2, SEkeyID.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) ||
		   (rc = sqlite3_step(sqlstmt.getstmt())) != SQLITE_DONE){
			   return -1;
		}
		return 0;
	} catch (...) {
		return -1;
	}
}

/* add entry to the table of non-encrypted telegram chat. return 0 on success, -1 otherwise. */
int setelegram::add_not_enc_table(string& chatID){
	try{
		statement sqlstmt;
		int rc = 0;
		if(chatID.empty() || !this->valid){
			return -1;
		}
		string query = "INSERT INTO ClearChat(chat_id) VALUES(?1);";
		if((sqlite3_prepare_v2(this->setelegram_db, query.c_str(), -1, sqlstmt.getstmtref(), nullptr) != SQLITE_OK) ||
		   (sqlite3_bind_text(sqlstmt.getstmt(), 1, chatID.c_str(), -1, SQLITE_STATIC) != SQLITE_OK)	||
		   (rc = sqlite3_step(sqlstmt.getstmt())) != SQLITE_DONE){
			   return -1;
		}
		return 0;
	} catch (...) {
		return -1;
	}
}
