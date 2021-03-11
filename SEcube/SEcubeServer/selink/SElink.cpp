/**
  ******************************************************************************
  * File Name          : SElink.cpp
  * Description        : Implementation of SElink library.
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

/*! \file  SElink.cpp
 *  \brief This file includes the implementation of SElink.
 *  \author Fornero Matteo
 *  \date 8/6/2020
 */

#include "SElink.h"
#include "../sekey/SEkey.h"
#include "base64/base64.h"
extern "C"{
#include "base64/cencode.h"
};

using namespace std;

void SElink::printdebug(){
	cout << "Ciphertext size: " << this->ciphertext.ciphertext_size << " bytes" << endl;
	if(this->ciphertext.algorithm == L1Algorithms::Algorithms::AES_HMACSHA256){
		cout << "Signature value: ";
		for(uint8_t x : this->ciphertext.digest){	printf("%02x", x);	}
		cout << endl;
	}
	if(this->ciphertext.algorithm == L1Algorithms::Algorithms::AES_HMACSHA256 || this->ciphertext.algorithm == L1Algorithms::Algorithms::HMACSHA256){
		cout << "Nonce PBKDF2 value: ";
		for(uint8_t x : this->ciphertext.digest_nonce){ printf("%02x", x); }
		cout << endl;
	}
	if((this->ciphertext.algorithm == L1Algorithms::Algorithms::AES_HMACSHA256 ||
		this->ciphertext.algorithm == L1Algorithms::Algorithms::AES) && this->ciphertext.mode == CryptoInitialisation::Modes::CTR){
		cout << "Nonce CTR value: ";
		for(uint8_t x : this->ciphertext.CTR_nonce){ printf("%02x", x); }
		cout << endl;
	}
	if((this->ciphertext.algorithm == L1Algorithms::Algorithms::AES_HMACSHA256 ||
		this->ciphertext.algorithm == L1Algorithms::Algorithms::AES) &&
	   (this->ciphertext.mode == CryptoInitialisation::Modes::CBC ||
		this->ciphertext.mode == CryptoInitialisation::Modes::CFB ||
		this->ciphertext.mode == CryptoInitialisation::Modes::OFB)){
		cout << "Initialization vector value: ";
		for(uint8_t x : this->ciphertext.initialization_vector){ printf("%02x", x); }
		cout << endl;
	}
}

void SElink::serialize(unique_ptr<char[]>& serialized_data, size_t& serialized_size){
	if(this->ciphertext.ciphertext == nullptr || this->ciphertext.ciphertext_size == 0){
		throw std::invalid_argument("The SEcube_ciphertext object does not contain a valid ciphertext.");
	}
	if(sizeof(char) != sizeof(uint8_t)){
		throw std::runtime_error("Base64 serialization is not supported on this machine because sizeof(char) != sizeof(uint8_t)."); // because we work on chars for base64 encoding and on unsigned char (uint8_t) for the object
		// notice that 99.9% of the times char and uint8_t have the same size (a char = 1 byte) but this is not always true
	}
	serialized_size = 0;
	size_t buffersize = (this->ciphertext.ciphertext_size + B5_SHA256_DIGEST_SIZE + SELINK_SERIAL_HEADER_SIZE);
	unique_ptr<uint8_t[]> buffer = make_unique<uint8_t[]>(buffersize);
	// step 1: copy the attributes of the ciphertext object to the buffer
	memcpy(buffer.get()+ALGORITHM, &(this->ciphertext.algorithm), 2);
	memcpy(buffer.get()+KEY_ID, &(this->ciphertext.key_id), 4);
	if(sizeof(this->ciphertext.ciphertext_size) <= 8){
		memcpy(buffer.get()+CIPHERTEXT_SIZE, &(this->ciphertext.ciphertext_size), sizeof(this->ciphertext.ciphertext_size));
	} else {
		throw std::runtime_error("Base64 serialization is not supported on this machine because sizeof(size_t) > 8 bytes.");
	}
	memcpy(buffer.get()+DIGEST_NONCE, this->ciphertext.digest_nonce.data(), B5_SHA256_DIGEST_SIZE);
	memcpy(buffer.get()+CTR_NONCE, this->ciphertext.CTR_nonce.data(), B5_AES_BLK_SIZE);
	memcpy(buffer.get()+DIGEST, this->ciphertext.digest.data(), B5_SHA256_DIGEST_SIZE);
	memcpy(buffer.get()+CIPHERTEXT, this->ciphertext.ciphertext.get(), this->ciphertext.ciphertext_size);
#ifdef SELINK_DEBUG
	cout << "BASE64 DEBUG INFO - Size of serialized buffer before Base64 encoding:" << buffersize << endl;
#endif
	// step 2: encode the buffer using Base64 to avoid problems with application at higher levels
	base64_encodestate s;
    base64_init_encodestate(&s);
    s.chars_per_line = 72; // check encode_b64() function, it must be the same number
	size_t required_encoded_size = base64_encode_length(buffersize, &s);
	if(required_encoded_size == 0){
		throw std::runtime_error("Runtime error while serializing the data. Try again.");
	}
	serialized_data = make_unique<char[]>(required_encoded_size);
	size_t encoded_size = encode_b64((const char*)buffer.get(), buffersize, serialized_data.get());
	if((encoded_size == 0) || (required_encoded_size != encoded_size)){
		throw std::runtime_error("Runtime error while serializing the data. Try again.");
	}
	serialized_size = encoded_size;
#ifdef SELINK_DEBUG
	cout << "BASE64 DEBUG INFO - Size of serialized buffer after Base64 encoding:" << *serialized_size << endl;
#endif
}

void SElink::deserialize(unique_ptr<char[]>& buffer_b64, size_t buffer_b64_size){
	if((buffer_b64 == nullptr) || (buffer_b64_size <= 0)){
		throw std::invalid_argument("Cannot deserialize a buffer that is NULL or empty.");
	}
	if(sizeof(char) != sizeof(uint8_t)){
		throw std::runtime_error("Base64 serialization is not supported on this machine because sizeof(char) != sizeof(uint8_t).");
	}
	// step 1: decode from base64 to plain binary
#ifdef SELINK_DEBUG
	cout << "Size of serialized buffer before Base64 decoding:" << buffer_b64_size << endl;
#endif
	unique_ptr<char[]> decoded = make_unique<char[]>(buffer_b64_size);
	size_t decoded_size = decode_b64(buffer_b64.get(), buffer_b64_size, decoded.get());
	if(decoded_size > buffer_b64_size || decoded_size < SELINK_SERIAL_HEADER_SIZE){
		throw std::runtime_error("Error while deserializing the buffer.");
	}
#ifdef SELINK_DEBUG
	cout << "Size of serialized buffer after Base64 decoding:" << decoded_size << endl;
#endif
	// step 2: copy the binary data into the SEcube_ciphertext object
	this->ciphertext.reset(); // clean the attributes of the SEcube_ciphertext
	memcpy(&(this->ciphertext.algorithm), decoded.get()+ALGORITHM, sizeof(this->ciphertext.algorithm));
	memcpy(&(this->ciphertext.key_id), decoded.get()+KEY_ID, sizeof(this->ciphertext.key_id));
	this->ciphertext.mode = CryptoInitialisation::Modes::CTR; // assigned manually because SElink always uses this mode
	if(sizeof(this->ciphertext.ciphertext_size) <= 8){
		memcpy(&(this->ciphertext.ciphertext_size), decoded.get()+CIPHERTEXT_SIZE, sizeof(this->ciphertext.ciphertext_size));
	} else {
		throw std::runtime_error("Error while deserializing the buffer.");
	}
	if((this->ciphertext.ciphertext_size + SELINK_SERIAL_HEADER_SIZE + B5_SHA256_DIGEST_SIZE) != decoded_size){
		throw std::runtime_error("Error while deserializing the buffer.");
	}
	for(int i=0; i<B5_SHA256_DIGEST_SIZE; i++){
		this->ciphertext.digest_nonce[i] = decoded[DIGEST_NONCE+i];
	}
	for(int i=0; i<B5_AES_BLK_SIZE; i++){
		this->ciphertext.CTR_nonce[i] = decoded[CTR_NONCE+i];
	}
	for(int i=0; i<B5_SHA256_DIGEST_SIZE; i++){
		this->ciphertext.digest[i] = decoded[DIGEST+i];
	}
	this->ciphertext.ciphertext = make_unique<uint8_t[]>(this->ciphertext.ciphertext_size);
	for(size_t i=0; i<this->ciphertext.ciphertext_size; i++){
		this->ciphertext.ciphertext[i] = decoded[CIPHERTEXT+i];
	}
}

int selink_encrypt_manual(L1 *l1, shared_ptr<uint8_t[]> plaintext, size_t plaintext_size, SElink& ciphertext, uint32_t key){
	if((plaintext == nullptr) || (plaintext_size == 0) || (l1 == nullptr)){
		return SELINK_ERR_PARAMS;
	}
	try{
		ciphertext.ciphertext.reset(); // the content of the ciphertext object is set to all 0s (it should already be empty unless the caller gives us an object that was used previously)
		l1->L1Encrypt(plaintext_size, plaintext, ciphertext.ciphertext, L1Algorithms::Algorithms::AES_HMACSHA256, CryptoInitialisation::Modes::CTR, key); // encrypt with L1 level API
	} catch(...) {
		return SELINK_ERR;
	}
	return SELINK_OK;
}

int selink_encrypt_auto(L1 *l1, shared_ptr<uint8_t[]> plaintext, size_t plaintext_size, SElink& ciphertext, vector<string>& recipient){
	if((plaintext == nullptr) || (plaintext_size == 0) || (recipient.empty()) || (l1 == nullptr)){
		return SELINK_ERR_PARAMS;
	}
	bool users = false, groups = false;
	uint32_t keyID;
	string dest, skeyID;
	// check if there are only users, only groups or users and groups as recipients
	for(vector<string>::iterator it=recipient.begin(); it!=recipient.end(); it++){
		if(check_input(*it, 0)){
			users = true;
		}
		if(check_input(*it, 1)){
			groups = true;
		}
	}
	if(!(users || groups)){ // invalid recipients
		return SELINK_ERR_PARAMS;
	}
	if(groups && users){ // having both users and groups as recipients is not valid
		return SELINK_ERR_PARAMS;
	}
	if(groups && (recipient.size()!=1)){ // if we want to send data to a group, only 1 group can be specified
		return SELINK_ERR_PARAMS;
	}
	// search the most secure key to be used
	if(users){ // if we want to send data to users (1 or many)
		if(recipient.size()==1){
			dest = recipient[0];
			int res = sekey_find_key_v1(skeyID, dest, se_key_type::symmetric_data_encryption); // API for a single user
			if(res != SEKEY_OK){
				return SELINK_ERR;
			}
		} else {
			int res = sekey_find_key_v3(skeyID, recipient, se_key_type::symmetric_data_encryption); // API for multiple users
			if(res != SEKEY_OK){
				return SELINK_ERR;
			}
		}
	}
	if(groups){ // if we want to send data to a group
		dest = recipient[0];
		int res = sekey_find_key_v2(skeyID, dest, se_key_type::symmetric_data_encryption); // API for a group
		if(res != SEKEY_OK){
			return SELINK_ERR;
		}
	}
	// retrieve number part of key ID (i.e. K123 -> 123)
	string ksub = skeyID.substr(1);
	keyID = stoul_wrap(ksub);
	// call manual API using retrieved key
	return selink_encrypt_manual(l1, plaintext, plaintext_size, ciphertext, keyID);
}

int selink_decrypt(L1 *l1, shared_ptr<uint8_t[]>& plaintext, size_t& plaintext_size, SElink& ciphertext){
	if((ciphertext.ciphertext.ciphertext_size == 0) || (ciphertext.ciphertext.ciphertext == nullptr) || (l1 == nullptr)){
		return SELINK_ERR_PARAMS;
	}
	try{
		l1->L1Decrypt(ciphertext.ciphertext, plaintext_size, plaintext);
	} catch(...) {
		return SELINK_ERR;
	}
	return SELINK_OK;
}
