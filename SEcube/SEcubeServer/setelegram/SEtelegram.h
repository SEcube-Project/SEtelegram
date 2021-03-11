/**
  ******************************************************************************
  * File Name          : SEtelegram.h
  * Description        : Header file for specific SEcube SDK APIs related to the SEtelegram demo.
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

#ifndef SETELEGRAM_SETELEGRAM_H_
#define SETELEGRAM_SETELEGRAM_H_

#include "../selink/SElink.h"

class setelegram{
private:
	sqlite3 *setelegram_db; // this is used to manage the local Telegram database (encrypted database on the SEcube MicroSD, containing the association Telegram chat ID -> SEkey ID)
	bool valid; // this must be checked after declaring a setelegram object, if false then the object creation was not completed correctly (the database connection does not work so the object can't be used)
public:
	setelegram(L0& l0, L1& l1); /* Use this constructor (telegram db automatically opened) */
	setelegram();
	~setelegram();
	int setelegram_find_chatID(std::string& chatID); /* Check if a Telegram chatID is stored into the database. */
	int setelegram_encrypt(L1 *l1, std::shared_ptr<uint8_t[]> plaintext, size_t plaintext_size, SElink& ciphertext, std::string& chatID); /* Encrypt Telegram message */
	int setelegram_decrypt(L1 *l1, std::shared_ptr<uint8_t[]>& plaintext, size_t& plaintext_size, SElink& ciphertext); /* Decrypt Telegram message */
	int add_enc_table(std::string& chatID, std::string& SEkeyID);
	int add_not_enc_table(std::string& chatID);
	bool isvalid();
};

#endif /* SETELEGRAM_SETELEGRAM_H_ */
