/**
  ******************************************************************************
  * File Name          : SElink.h
  * Description        : Header for SElink library.
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

/*! \file  SElink.h
 *  \brief This file includes every function, class and definition about SElink.
 *  \author Fornero Matteo
 *  \date 8/6/2020
 */

#ifndef SELINK_SELINK_H_
#define SELINK_SELINK_H_

#include "../sources/L1/L1.h"

#define SELINK_SERIAL_HEADER_SIZE 94 /**< This is the quantity of data that must be prepended to the actual data in order to serialize them and send them (i.e. over a TCP connection).
										  2 bytes for algorithm ID, 4 bytes for key ID, 32 bytes for PBKDF2 nonce, 16 bytes for CTR NONCE, 8 bytes for ciphertext size, 32 bytes for signature. */

/** These are simply the values that are returned by the APIs of SElink. */
enum selink_return_values{
	SELINK_OK = 0, /**< Everything went well. */
	SELINK_ERR_PARAMS = 1, /**< The caller passed wrong parameters. */
	SELINK_CORRUPTED_DATA = 2, /**< This is returned only on the receiver side of a communication, whenever the decryption function of SElink detects a mismatch between the signature of the data sent by the sender and the signature recomputed by
	the receiver. This means that the data are corrupt, so they must not be used. Possible reasons for corruption: error transmitting/receiving data, external attack (i.e. a hacker that injected random bits in the data without really
	knowing what he was changing since data are encrypted), etc. */
	SELINK_ERR = 3 /**< Generic error message. */
};

/** These value are used internally by SElink to serialize and deserialize the data. These are offsets to specific data inside the array of bytes that is serialized and deserialized. */
enum selink_serial_offset{
	ALGORITHM = 0, /**< Encryption algorithm. */
	KEY_ID = 2, /**< Key ID. */
	CIPHERTEXT_SIZE = 6, /**< Size of encrypted data. */
	DIGEST_NONCE = 14, /**< Nonce. */
	CTR_NONCE = 46, /**< Another nonce. */
	DIGEST = 62, /**< Signature of data. */
	CIPHERTEXT = 94 /**< Encrypted data. */
};

class SElink{
public:
	SEcube_ciphertext ciphertext;
	void serialize(std::unique_ptr<char[]>& serialized_data, size_t& serialized_size); /**< This function is used to serialize the ciphertext stored by an object of this class. Returns NULL in case of error. The serialized data is expressed in base64 format. */
	void deserialize(std::unique_ptr<char[]>& buffer_b64, size_t buffer_b64_size); /**< This function is used to deserialize a ciphertext object that was previously serialized to base64 format. Returns 0 upon success, -1 in case of error. */
	void printdebug(); /**< This function is simply used to print some info useful to debug SElink. */
};

/** @brief Encrypt certain data using a key decided by the caller. This must be used only when you don't want to use SEkey.
 * @param [in] l1 The L1 object created to communicate with the SEcube (i.e. to login).
 * @param [in] plaintext A pointer to an array of bytes where the plaintext is stored.
 * @param [in] plaintext_size How many bytes are stored in the plaintext.
 * @param [out] ciphertext A pointer to an existing selink_ciphertext object where the actual ciphertext will be stored.
 * @param [in] key The ID (integer value) of the key to be used to encrypt the data.
 * @return A value from \ref selink_return_values.
 * @details The attributes of the ciphertext object are automatically filled by this API. Encryption is done using AES256-HMAC-SHA256, once the data are successfully
 * encrypted they should be serialized before being sent to the recipient. */
int selink_encrypt_manual(L1 *l1, std::shared_ptr<uint8_t[]> plaintext, size_t plaintext_size, SElink& ciphertext, uint32_t key);

/** @brief Encrypt certain data using a key automatically computed by SEkey given the list of the recipients.
 * @param [in] l1 The L1 object created to communicate with the SEcube (i.e. to login).
 * @param [in] plaintext A pointer to an array of bytes where the plaintext is stored.
 * @param [in] plaintext_size How many bytes are stored in the plaintext.
 * @param [out] ciphertext A pointer to an existing selink_ciphertext object where the actual ciphertext will be stored.
 * @param [in] recipient A list of recipients, for example a list of users who are supposed to receive the encrypted data.
 * @return A value from \ref selink_return_values.
 * @details The attributes of the ciphertext object are automatically filled by this API. Encryption is done using AES256-HMAC-SHA256, once the data are successfully
 * encrypted they should be serialized before being sent to the recipient(s). Notice that the list of the recipients can include a single user or multiple users, alternatively
 * the caller can also specify a group. The users and the groups are identified by their IDs, the same IDs that are used by SEkey to manage the distribution of the keys and the
 * security policies. As an example, a possible list of recipients may include "U1, U2, U3", another possibility is a single user such as "U2", another possibility is a single
 * group such as "G2". Notice that multiple groups cannot be specified (because this violates the rules of SEkey); similarly, if multiple users are specified they must have at least
 * a common group otherwise this API will fail. This API internally exploits the APIs sekey_find_key_v1(), sekey_find_key_v2() and sekey_find_key_v3(); be sure to check their documentation. */
int selink_encrypt_auto(L1 *l1, std::shared_ptr<uint8_t[]> plaintext, size_t plaintext_size, SElink& ciphertext, std::vector<std::string>& recipient);

/** @brief Decrypt a selink_ciphertext object.
 * @param [in] l1 The L1 object created to communicate with the SEcube (i.e. to login).
 * @param [out] plaintext A pointer to a vector of bytes where the plaintext will be stored.
 * @param [out] plaintext_size How many bytes are stored in the plaintext.
 * @param [in] ciphertext An existing selink_ciphertext object where the actual ciphertext is stored.
 * @return A value from \ref selink_return_values.
 * @details The ciphertext will be automatically decrypted using the attributes of the selink_ciphertext object itself. An integrity and authentication check will be performed on the decrypted data, in case of mismatch,
 * the caller will be notified with a specific error. */
int selink_decrypt(L1 *l1, std::shared_ptr<uint8_t[]>& plaintext, size_t& plaintext_size, SElink& ciphertext);

#endif /* SELINK_SELINK_H_ */
