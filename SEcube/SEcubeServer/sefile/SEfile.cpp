/**
  ******************************************************************************
  * File Name          : SEfile.cpp
  * Description        : SEfile library implementation.
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

/** \file SEfile.cpp
 *  \brief In this file you will find the implementation of the functions already described in \ref SEfile.h and \ref SEfile_C_interface.h
 *   \authors Matteo Fornero, Francesco Giavatto, Nicolo Maunero, Giulio Scalia
 *   \date 17/09/2016
 */

//#ifdef __linux__
//#define _GNU_SOURCE // enable this if it is not enabled by default on your machine (needed for libstdc++)
//#endif

#include "environment.h"
#include "SEfile.h"
#include <time.h>

#define USING_SEKEY // comment this if you do not want to use SEkey (i.e. you only use SEfile)
#ifdef USING_SEKEY
#include "../sekey/SEkey.h"
#endif

bool override_key_check = false;

SEFILE_SECTOR::SEFILE_SECTOR(){
	/* with this constructor we simply want to initialize to zeros the entire memory used by this structure */
	this->len = 0;
	memset(this->signature, 0, 32);
	memset(this->data, 0, SEFILE_LOGIC_DATA);
}

SEFILE_HANDLE::SEFILE_HANDLE(){
	log_offset = 0;
#if defined(__linux__) || defined(__APPLE__)
	fd = -1;
#elif _WIN32
    fd = INVALID_HANDLE_VALUE;
#endif
	memset(nonce_ctr, 0, 16);
	memset(nonce_pbkdf2, 0, SEFILE_NONCE_LEN);
	memset(name, '\0', MAX_PATHNAME);
	/* notice that we do not specify a destructor because we do not really need it. the constructor instead is useful to set initial values for the file descriptors
	 * and to clear the buffer used to contain the name of the file (if it is a SQLite database file, otherwise it is not used) */
}

SEfile::SEfile(){
	this->EnvCrypto = 0;
	this->EnvKeyID = 0;
	this->LastDecryptCheckTime = 0;
	this->LastEncryptCheckTime = 0;
	this->IsOpen = false;
	this->l1 = nullptr;
	this->handleptr = std::make_shared<SEFILE_HANDLE>();
};

SEfile::SEfile(L1* secube){
	this->EnvCrypto = 0;
	this->EnvKeyID = 0;
	this->LastDecryptCheckTime = 0;
	this->LastEncryptCheckTime = 0;
	this->IsOpen = false;
	this->l1 = secube;
	this->handleptr = std::make_shared<SEFILE_HANDLE>();
};

SEfile::SEfile(L1* secube, uint32_t keyID){
	this->EnvCrypto = 0;
	this->EnvKeyID = keyID;
	this->LastDecryptCheckTime = 0;
	this->LastEncryptCheckTime = 0;
	this->IsOpen = false;
	this->l1 = secube;
	this->handleptr = std::make_shared<SEFILE_HANDLE>();
};

SEfile::SEfile(L1* secube, uint32_t keyID, uint16_t crypto){
	this->EnvCrypto = crypto;
	this->EnvKeyID = keyID;
	this->LastDecryptCheckTime = 0;
	this->LastEncryptCheckTime = 0;
	this->IsOpen = false;
	this->l1 = secube;
	this->handleptr = std::make_shared<SEFILE_HANDLE>();
};

SEfile::~SEfile(){
	this->secure_close();
	this->secure_finit();
};
uint16_t SEfile::secure_key_check(uint16_t direction){
#ifndef USING_SEKEY
	return 0;
#else
	if(override_key_check){ return 0; }
	if(keyIDclass(this->EnvKeyID) != L1Key::IdClass::KMS){ // key ID is not among the keys managed by SEkey
		return 0;
	}
	if((direction != CryptoInitialisation::Direction::ENCRYPT) && (direction != CryptoInitialisation::Direction::DECRYPT)){
		return 1;
	}
    if(keyIDclass(this->EnvKeyID) == L1Key::IdClass::KMS){
    	if(!SEkey_running || (sekey_check_expired_keys() != SEKEY_OK)){
    		return 1;
    	}
        uint16_t retvalue = 0;
        se_key key;
        std::string k = "K" + std::to_string(this->EnvKeyID);
        time_t currtime = time(nullptr);
    	if(direction == CryptoInitialisation::Direction::ENCRYPT){
        	if((this->LastEncryptCheckTime != 0) && (difftime(currtime, this->LastEncryptCheckTime) < KEY_CHECK_INTERVAL)){
        		return 0;
        	}
        	if(sekey_key_get_info(k, &key) != SEKEY_OK){
        		this->LastEncryptCheckTime = 0;
        		return 1;
        	}
        	se_key_status status = key.get_status();
			switch(status){
				case se_key_status::statusmin:
					retvalue = SEKEY_INVALID_KEY;
					this->LastEncryptCheckTime = 0;
					break;
				case se_key_status::statusmax:
					retvalue = SEKEY_INVALID_KEY;
					this->LastEncryptCheckTime = 0;
					break;
				case se_key_status::preactive:
					retvalue = SEKEY_PREACTIVE_KEY;
					this->LastEncryptCheckTime = 0;
					break;
				case se_key_status::suspended:
					retvalue = SEKEY_SUSPENDED_KEY;
					this->LastEncryptCheckTime = 0;
					break;
				case se_key_status::deactivated:
					retvalue = SEKEY_DEACTIVATED_KEY;
					this->LastEncryptCheckTime = 0;
					break;
				case se_key_status::compromised:
					retvalue = SEKEY_COMPROMISED_KEY;
					this->LastEncryptCheckTime = 0;
					break;
				case se_key_status::destroyed:
					retvalue = SEKEY_DESTROYED_KEY;
					this->LastEncryptCheckTime = 0;
					break;
				case se_key_status::active:
					retvalue = 0;
					this->LastEncryptCheckTime = currtime;
					break;
				default:
					this->LastEncryptCheckTime = 0;
					retvalue = 1;
			}
    	}
    	if(direction == CryptoInitialisation::Direction::DECRYPT){
        	if((this->LastDecryptCheckTime != 0) && (difftime(currtime, this->LastDecryptCheckTime) < KEY_CHECK_INTERVAL)){
        		return 0;
        	}
        	if(sekey_key_get_info(k, &key) != SEKEY_OK){
        		this->LastDecryptCheckTime = 0;
        		return 1;
        	}
        	se_key_status status = key.get_status();
			switch(status){
				case se_key_status::statusmin:
					retvalue = SEKEY_INVALID_KEY;
					this->LastDecryptCheckTime = 0;
					break;
				case se_key_status::statusmax:
					retvalue = SEKEY_INVALID_KEY;
					this->LastDecryptCheckTime = 0;
					break;
				case se_key_status::preactive:
					retvalue = SEKEY_PREACTIVE_KEY;
					this->LastDecryptCheckTime = 0;
					break;
				case se_key_status::suspended:
					retvalue = 0;
					this->LastDecryptCheckTime = currtime;
					break;
				case se_key_status::active:
					retvalue = 0;
					this->LastDecryptCheckTime = currtime;
					break;
				case se_key_status::deactivated:
					retvalue = 0;
					this->LastDecryptCheckTime = currtime;
					break;
				case se_key_status::compromised:
					retvalue = SEKEY_COMPROMISED_KEY;
					this->LastDecryptCheckTime = 0;
					break;
				case se_key_status::destroyed:
					retvalue = SEKEY_DESTROYED_KEY;
					this->LastDecryptCheckTime = 0;
					break;
				default:
					this->LastDecryptCheckTime = 0;
					retvalue = 1;
			}
		}
    	return retvalue;
    } else {
    	return 0;
    }
#endif
}

uint16_t SEfile::secure_init(L1 *l1ptr, uint32_t keyID, uint16_t crypto){
	//uint16_t count = 0;
    std::vector<se3Algo> algTable;
    if((l1ptr == nullptr) || (this->EnvCrypto != 0) || (this->EnvKeyID != 0) || (this->l1 != nullptr) || (keyID==0)){ // already initialized or no secube pointer
        return SEFILE_ENV_INIT_ERROR;
    }
    if(!l1ptr->L1GetSessionLoggedIn()){
        return SEFILE_ENV_INIT_ERROR;
    }
    /* secube L1 pointer part */
    l1 = l1ptr;
    /* key part */
	bool b = false;
	l1ptr->L1FindKey(keyID, b);
    if(!b){ // this is in case we can't find the key
		secure_finit();
		return SEFILE_ENV_INIT_ERROR;
	} else {
		this->EnvKeyID = keyID;
	}
	/* algorithm part */
    if(crypto < (L1Algorithms::Algorithms::ALGORITHM_MAX)){ // this is used if we already know which algorithm to use
        try{
        	l1ptr->L1GetAlgorithms(algTable);
        } catch (...){
        	secure_finit();
        	return SEFILE_ENV_INIT_ERROR;
        }
        if(/*count > 0 &&*/ algTable.size() > crypto && algTable.at(crypto).type == L1Crypto::CryptoTypes::SE3_CRYPTO_TYPE_BLOCKCIPHER_AUTH){
            EnvCrypto = crypto;
        } else {
        	secure_finit();
        	return SEFILE_ENV_INIT_ERROR;
        }
    } else { // this is used when we don't know which algorithm to use, in this case we use the first suitable algorithm
		secure_finit();
		return SEFILE_ENV_INIT_ERROR;
    }
    return 0;
}

void SEfile::secure_finit(){
	this->EnvCrypto = 0;
	this->EnvKeyID = 0;
    this->l1 = nullptr;
}

uint16_t SEfile::secure_open(char *path, int32_t mode, int32_t creation){
    if((path == nullptr) || (this->l1 == nullptr)){
    	return SEFILE_OPEN_ERROR;
    }
	uint16_t commandError=0;
    char enc_filename[MAX_PATHNAME];
    uint16_t lenc=0;
    std::shared_ptr<SEFILE_HANDLE> hTmp = std::make_shared<SEFILE_HANDLE>();
    if(hTmp == nullptr){ return SEFILE_OPEN_ERROR; }
    SEFILE_SECTOR buffEnc, buffDec;
	#ifdef _WIN32
    DWORD nBytesRead = 0;
	#endif
    memset(enc_filename, 0, MAX_PATHNAME*sizeof(char));
    if(creation==(SEFILE_NEWFILE)){ // in this case the file must be created
    	if((commandError = this->secure_create(path, hTmp, mode))!=0){
        	this->handleptr = std::move(hTmp);
        	this->secure_close();
    		return SEFILE_OPEN_ERROR;
        }
        this->handleptr = std::move(hTmp);
        this->IsOpen = true;
        return 0;
    }
    // in this other case the file must be simply opened, it already exists on the disk
    if(crypto_filename(path, enc_filename, &lenc)){
        return SEFILE_OPEN_ERROR;
    }
    if(hTmp==nullptr){
        return SEFILE_OPEN_ERROR;
    }
    /* open phase start */
	#if defined(__linux__) || defined(__APPLE__)
    if((hTmp->fd = open(enc_filename, mode | creation, S_IRWXU)) == -1 ){
    	return SEFILE_OPEN_ERROR;
    }
    #elif _WIN32
    hTmp->fd = CreateFile(
                enc_filename,		              				// file to open
                mode,   	     								// open for reading and writing
                FILE_SHARE_READ | FILE_SHARE_WRITE,       		// share
                nullptr,                  							// default security
                creation,	      								// existing file only
                FILE_ATTRIBUTE_NORMAL,                          // normal file
                nullptr);                 							// no attr. template
    if (hTmp->fd == INVALID_HANDLE_VALUE){
        //DWORD errValue = GetLastError();
    	return SEFILE_OPEN_ERROR;
    }
	#endif
    /* open phase end */
#if defined(__linux__) || defined(__APPLE__)
    if (read(hTmp->fd, &buffEnc, sizeof(SEFILE_SECTOR)) != sizeof(SEFILE_SECTOR)){
    	this->handleptr = std::move(hTmp);
    	this->secure_close();
    	return SEFILE_OPEN_ERROR;
    }
    hTmp->log_offset = lseek(hTmp->fd, 0, SEEK_CUR);
#elif _WIN32
    if (ReadFile(hTmp->fd, &buffEnc, sizeof(SEFILE_SECTOR), &nBytesRead, nullptr) == FALSE && nBytesRead != sizeof(SEFILE_SECTOR)){
    	this->handleptr = std::move(hTmp);
    	this->secure_close();
    	return SEFILE_OPEN_ERROR;
    }
    hTmp->log_offset = SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT);
#endif
    this->EnvKeyID = buffEnc.header.key_header.key_id; // assign key to be used (must be done before crypt_header is called)
    this->EnvCrypto = buffEnc.header.key_header.algorithm;
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::DECRYPT); // check if the key is valid
    if(rc){ // return if the key is not valid
    	this->handleptr = std::move(hTmp);
    	this->secure_close();
    	return rc;
    }
    // decrypt the header
    if (this->crypt_header(&buffEnc, &buffDec, SEFILE_SECTOR_DATA_SIZE, CryptoInitialisation::Direction::DECRYPT)){
    	this->handleptr = std::move(hTmp);
    	this->secure_close();
    	return SEFILE_OPEN_ERROR;
    }
    memcpy(hTmp->nonce_ctr, buffDec.header.nonce_ctr, 16);
    memcpy(hTmp->nonce_pbkdf2, buffDec.header.nonce_pbkdf2, SEFILE_NONCE_LEN);
    this->handleptr = std::move(hTmp);
    if(commandError == 0){
    	this->IsOpen = true;
    }
    return commandError;
}

uint16_t SEfile::secure_create(char *path, std::shared_ptr<SEFILE_HANDLE> hFile, int mode){
    if((path == nullptr) || (hFile == nullptr) || (this->l1 == nullptr) || (this->EnvKeyID == 0) || (this->EnvCrypto != L1Algorithms::Algorithms::AES_HMACSHA256)){
    	return SEFILE_CREATE_ERROR;
    }
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::ENCRYPT); // check if the key is valid
    if(rc){ return rc; } // return if the key is not valid
	uint16_t commandError=0;
    char enc_filename[MAX_PATHNAME], *filename=nullptr;
    uint16_t lenc=0;
    std::unique_ptr<SEFILE_SECTOR> buff = std::make_unique<SEFILE_SECTOR>();
    std::unique_ptr<SEFILE_SECTOR> buffEnc = std::make_unique<SEFILE_SECTOR>();
    size_t random_padding = 0;
    uint8_t *padding_ptr = nullptr;
#ifdef _WIN32
    DWORD nBytesWritten = 0;
#endif
    memset(enc_filename, 0, MAX_PATHNAME*sizeof(char));
    if(crypto_filename(path, enc_filename, ((uint16_t*)&lenc))){
        return SEFILE_CREATE_ERROR;
    }
    if((buff == nullptr) || (buffEnc == nullptr)){
        return SEFILE_CREATE_ERROR;
    }
    /* create phase start */
#if defined(__linux__) || defined(__APPLE__)
    if((hFile->fd = open(enc_filename, mode | O_CREAT | O_TRUNC , S_IRWXU)) == -1 ){
        return SEFILE_CREATE_ERROR;
    }
#elif _WIN32
    hFile->fd = CreateFile(
    			enc_filename,		          					// file to open
                mode,     	     								// open for reading and writing
                FILE_SHARE_READ | FILE_SHARE_WRITE,             // share
                nullptr,                  							// default security
                CREATE_ALWAYS,	      							// existing file only
                FILE_ATTRIBUTE_NORMAL,                          // normal file
                nullptr);                 							// no attr. template
    if (hFile->fd == INVALID_HANDLE_VALUE){
        return SEFILE_CREATE_ERROR;
    }
#endif
    /* create phase end */
    /* create new header sector start ****************************************/
    L0Support::Se3Rand(16, buff->header.nonce_ctr);
    memcpy(hFile->nonce_ctr, buff->header.nonce_ctr, 16);
    L0Support::Se3Rand(SEFILE_NONCE_LEN, buff->header.nonce_pbkdf2);
    memcpy(hFile->nonce_pbkdf2, buff->header.nonce_pbkdf2, SEFILE_NONCE_LEN);
    buff->header.uid=0;
    buff->header.uid_cnt=0;
    buff->header.ver=0;
    buff->header.magic=0;
    buff->header.key_header.key_id = this->EnvKeyID; // assign the required value to the key ID attribute (the encryption key used for this file)
    buff->header.key_header.algorithm = this->EnvCrypto;
    memset(buff->header.key_header.padding, 0, 10); // set padding to 0
    /* part inserted to support re-encryption of compromised files */
    std::string clean_path(path); // build string from incoming char*
    std::string tmps(".reencryptedsefile"); // remove this substring from path (so this is set in the header)
    if((clean_path.length() > tmps.length()) && (clean_path.compare(clean_path.length() - tmps.length(), tmps.length(), tmps) == 0)){
    	clean_path.erase(clean_path.length() - tmps.length());
    }
    /* end */
    filename=strrchr((char*)clean_path.c_str(), '/');
    if(filename==nullptr){
        filename=strrchr((char*)clean_path.c_str(), '\\');
        if(filename==nullptr) filename = (char*)clean_path.c_str();
        else filename++;
    }else{
        filename++;
    }
    buff->header.fname_len=strlen(filename);
    buff->len=0;
    //copy filename in header
    memcpy(buff->data+sizeof(SEFILE_HEADER), filename, buff->header.fname_len);
    padding_ptr = (buff->data + sizeof(SEFILE_HEADER) + buff->header.fname_len);
    random_padding = (buff->data+SEFILE_LOGIC_DATA) - padding_ptr;
    L0Support::Se3Rand(random_padding, padding_ptr);
    if (this->crypt_header(buff.get(), buffEnc.get(), SEFILE_SECTOR_DATA_SIZE, CryptoInitialisation::Direction::ENCRYPT)){
    	return SEFILE_CREATE_ERROR;
    }
#if defined(__linux__) || defined(__APPLE__)
    if(write(hFile->fd, buffEnc.get(), sizeof(SEFILE_SECTOR)) != sizeof(SEFILE_SECTOR) ){
    	this->secure_close();
    	return SEFILE_CREATE_ERROR;
    }
#elif _WIN32
    if (WriteFile(hFile->fd, buffEnc.get(), sizeof(SEFILE_SECTOR), &nBytesWritten, nullptr) == FALSE){
    	return SEFILE_CREATE_ERROR;
    }
#endif
    /* move pointer after the first sector */
#if defined(__linux__) || defined(__APPLE__)
    hFile->log_offset = lseek(hFile->fd, 0, SEEK_CUR);
#elif _WIN32
    hFile->log_offset = SetFilePointer(hFile->fd, 0, nullptr, FILE_CURRENT);
#endif
    if(commandError == 0){
    	this->IsOpen = true;
    }
    return commandError;
}

uint16_t SEfile::secure_write(uint8_t * dataIn, uint32_t dataIn_len){
    if((this->IsOpen == false) || (this->handleptr == nullptr) || (this->l1 == nullptr)){
    	return SEFILE_WRITE_ERROR;
    }
    if(dataIn_len == 0){ return 0; }
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::ENCRYPT); // check if the key is valid
    if(rc){ return rc; } // return if the key is not valid
	std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
    int32_t absOffset=0, sectOffset=0;
    std::unique_ptr<SEFILE_SECTOR> cryptBuff = std::make_unique<SEFILE_SECTOR>();
    std::unique_ptr<SEFILE_SECTOR> decryptBuff = std::make_unique<SEFILE_SECTOR>();
    int length = 0;
    size_t current_position = SEFILE_SECTOR_SIZE;
    size_t random_padding = 0;
    uint8_t *padding_ptr = nullptr;
#if defined(__linux__) || defined(__APPLE__)
    int nBytesRead=0;
#elif _WIN32
    DWORD nBytesWritten=0, nBytesRead=0;
#endif
    if((cryptBuff == nullptr) || (decryptBuff == nullptr)){
        return SEFILE_WRITE_ERROR;
    }
    //    if (secure_sync(hFile)){
    //        return SEFILE_WRITE_ERROR;
    //    }
    //move the pointer to the begin of the sector
#if defined(__linux__) || defined(__APPLE__)
    if((absOffset=lseek(hTmp->fd, 0, SEEK_CUR))<0 || absOffset!=hTmp->log_offset){
        return SEFILE_WRITE_ERROR;
    }
    current_position = lseek(hTmp->fd,((int32_t)(absOffset/SEFILE_SECTOR_SIZE))*SEFILE_SECTOR_SIZE, SEEK_SET);
#elif _WIN32
    if((absOffset=SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT))<0 || ((uint32_t)absOffset)!=hTmp->log_offset){
        return SEFILE_WRITE_ERROR;
    }
    current_position = SetFilePointer(hTmp->fd,((int32_t)(absOffset/SEFILE_SECTOR_SIZE))*SEFILE_SECTOR_SIZE, nullptr, FILE_BEGIN);
#endif
    //save the relative position inside the sector
    sectOffset = absOffset % SEFILE_SECTOR_SIZE;
    //read the whole sector and move back the pointer
#if defined(__linux__) || defined(__APPLE__)
    nBytesRead=read(hTmp->fd, cryptBuff.get(), SEFILE_SECTOR_SIZE);
#elif _WIN32
    ReadFile(hTmp->fd, cryptBuff.get(), SEFILE_SECTOR_SIZE, &nBytesRead, nullptr);
#endif
    if(nBytesRead>0){
        if (this->decrypt_sectors(cryptBuff.get(), decryptBuff.get(), SEFILE_SECTOR_DATA_SIZE, pos_to_cipher_block(current_position), hTmp->nonce_ctr, hTmp->nonce_pbkdf2)){
            return SEFILE_WRITE_ERROR;
        }
        //sector integrity check
        if (memcmp(cryptBuff->signature, decryptBuff->signature, B5_SHA256_DIGEST_SIZE)){
            return SEFILE_SIGNATURE_MISMATCH;
        }
		#if defined(__linux__) || defined(__APPLE__)
		lseek(hTmp->fd, (-1)*SEFILE_SECTOR_SIZE, SEEK_CUR);
		#elif _WIN32
		SetFilePointer(hTmp->fd, (-1)*SEFILE_SECTOR_SIZE, nullptr, FILE_CURRENT);
		#endif
    }else{
        //sector empty
        decryptBuff->len=0;
    }
    do{
        //fill the sector with input data until datain are over or the sector is full
        //length = dataIn_len < (SEFILE_LOGIC_DATA-sectOffset) ? dataIn_len : SEFILE_LOGIC_DATA-sectOffset;
        if(dataIn_len < (SEFILE_LOGIC_DATA-sectOffset)){
        	length = dataIn_len;
        } else {
        	length = (SEFILE_LOGIC_DATA-sectOffset);
        }
        memcpy(decryptBuff->data+sectOffset, dataIn, length);
        //update sector data length if needed
        if( (length + (sectOffset)) > decryptBuff->len){
            decryptBuff->len = length + sectOffset;
        }
        /*Padding must be random! (known plaintext attack)*/
        padding_ptr = decryptBuff->data + decryptBuff->len;
        random_padding = decryptBuff->data + SEFILE_LOGIC_DATA - padding_ptr;
        L0Support::Se3Rand(random_padding, padding_ptr);
        //encrypt sector
        if (this->crypt_sectors(decryptBuff.get(), cryptBuff.get(), SEFILE_SECTOR_DATA_SIZE, pos_to_cipher_block(current_position), hTmp->nonce_ctr, hTmp->nonce_pbkdf2)){
            return SEFILE_WRITE_ERROR;
        }
        /* writeback sector into file phase start */
		#if defined(__linux__) || defined (__APPLE__)
        if(write(hTmp->fd, cryptBuff.get(), SEFILE_SECTOR_SIZE) != SEFILE_SECTOR_SIZE){
            return SEFILE_WRITE_ERROR;
        }
		#elif _WIN32
        if (WriteFile(hTmp->fd, cryptBuff.get(), SEFILE_SECTOR_SIZE, &nBytesWritten, nullptr) == FALSE){
            return SEFILE_WRITE_ERROR;
        }
        if(nBytesWritten != (DWORD) SEFILE_SECTOR_SIZE){
            return SEFILE_WRITE_ERROR;
        }
		#endif
        /* writeback sector into file phase end */
        current_position += SEFILE_SECTOR_SIZE;
        dataIn_len-=length;
        dataIn+=length;
        sectOffset = (sectOffset+length)%(SEFILE_LOGIC_DATA);
        decryptBuff->len=0;
    } while(dataIn_len>0); //cycles unless all dataIn are processed
    //move the pointer inside the last sector written
    if(sectOffset!=0){
#if defined(__linux__) || defined(__APPLE__)
        hTmp->log_offset=lseek(hTmp->fd, (sectOffset - SEFILE_SECTOR_SIZE), SEEK_CUR);
#elif _WIN32
        hTmp->log_offset = SetFilePointer(hTmp->fd, (LONG)(sectOffset - SEFILE_SECTOR_SIZE), nullptr, FILE_CURRENT);
#endif
    }else{
#if defined(__linux__) || defined(__APPLE__)
        hTmp->log_offset=lseek(hTmp->fd, 0, SEEK_CUR);
#elif _WIN32
        hTmp->log_offset=SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT);
#endif
    }
    return 0;
}

uint16_t SEfile::secure_read(uint8_t * dataOut, uint32_t dataOut_len, uint32_t *bytesRead){
    if((bytesRead == nullptr) || (this->l1 == nullptr) || (this->handleptr == nullptr) || (this->IsOpen == false)){
    	return SEFILE_READ_ERROR;
    }
    if (dataOut_len == 0){ return 0; }
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::DECRYPT); // check if the key is valid
    if(rc){ return rc; } // return if the key is not valid
	std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
    int32_t absOffset=0, sectOffset=0;
    uint32_t dataRead=0;
    std::unique_ptr<SEFILE_SECTOR> cryptBuff = std::make_unique<SEFILE_SECTOR>();
    std::unique_ptr<SEFILE_SECTOR> decryptBuff = std::make_unique<SEFILE_SECTOR>();
    int length = 0;
    size_t current_position = SEFILE_SECTOR_SIZE;
    int32_t data_remaining = 0;
    *bytesRead = 0; // no bytes read yet
#if defined(__linux__) || defined(__APPLE__)
    int nBytesRead=0;
#elif _WIN32
    DWORD nBytesRead=0;
#endif
    if(cryptBuff==nullptr || decryptBuff==nullptr){
        return SEFILE_READ_ERROR;
    }
    //    if (secure_sync(hFile)){
    //        return SEFILE_WRITE_ERROR;
    //    }
    //move the pointer to the begin of the sector
#if defined(__linux__) || defined(__APPLE__)
    if((absOffset=lseek(hTmp->fd, 0, SEEK_CUR))<0 || absOffset!=hTmp->log_offset){
        return SEFILE_READ_ERROR;
    }
    current_position = lseek(hTmp->fd,((int32_t)(absOffset/SEFILE_SECTOR_SIZE))*SEFILE_SECTOR_SIZE, SEEK_SET);
#elif _WIN32
    if((absOffset=SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT))<0 || ((uint32_t)absOffset) !=hTmp->log_offset){
        return SEFILE_READ_ERROR;
    }
    current_position = SetFilePointer(hTmp->fd, ((int32_t)(absOffset / SEFILE_SECTOR_SIZE))*SEFILE_SECTOR_SIZE, nullptr, FILE_BEGIN);
#endif
    //save the relative position inside the sector
    sectOffset=absOffset%SEFILE_SECTOR_SIZE;
    do{
        //read the whole sector
#if defined(__linux__) || defined(__APPLE__)
        nBytesRead=read(hTmp->fd, cryptBuff.get(), SEFILE_SECTOR_SIZE);
#elif _WIN32
        ReadFile(hTmp->fd, cryptBuff.get(), SEFILE_SECTOR_SIZE, &nBytesRead, nullptr);
#endif
        if(nBytesRead>0){
            if(this->decrypt_sectors(cryptBuff.get(), decryptBuff.get(), SEFILE_SECTOR_DATA_SIZE, pos_to_cipher_block(current_position), hTmp->nonce_ctr, hTmp->nonce_pbkdf2)){
                return SEFILE_READ_ERROR;
            }
            //sector integrity check
            if(memcmp(cryptBuff->signature, decryptBuff->signature, B5_SHA256_DIGEST_SIZE)){
                return SEFILE_SIGNATURE_MISMATCH;
            }
        }else{
            break;
        }
        data_remaining = (decryptBuff->len) - sectOffset; //remaining data in THIS sector
        length = dataOut_len < (SEFILE_LOGIC_DATA-sectOffset) ? dataOut_len : (SEFILE_LOGIC_DATA-sectOffset);
        if(data_remaining<length){
            length = data_remaining;
        }
        memcpy(dataOut+dataRead, decryptBuff->data+sectOffset, length);
        current_position += SEFILE_SECTOR_SIZE;
        dataOut_len-=length;
        dataRead+=length;
        sectOffset=(sectOffset+length)%SEFILE_LOGIC_DATA;
    }while(dataOut_len>0); //cycles unless all data requested are read
    //move the pointer inside the last sector read
    if(sectOffset!=0){
#if defined(__linux__) || defined(__APPLE__)
        hTmp->log_offset=lseek(hTmp->fd, (-1)*(SEFILE_SECTOR_SIZE-sectOffset), SEEK_CUR); //todo check sectoffset
#elif _WIN32
        hTmp->log_offset=SetFilePointer(hTmp->fd,(-1)*(SEFILE_SECTOR_SIZE-sectOffset), nullptr, FILE_CURRENT);
#endif
    }else{
#if defined(__linux__) || defined(__APPLE__)
        hTmp->log_offset=lseek(hTmp->fd, 0, SEEK_CUR); //todo check sectoffset
#elif _WIN32
        hTmp->log_offset=SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT);
#endif
    }
    *bytesRead=dataRead;
    return 0;
}

uint16_t SEfile::secure_seek(int32_t offset, int32_t *position, uint8_t whence){
    if((this->l1 == nullptr) || (this->handleptr == nullptr) || (this->IsOpen == false)){
    	return SEFILE_SEEK_ERROR;
    }
	int32_t dest=0, tmp=0, buffer_size=0;
    int32_t overhead=0, absOffset=0, sectOffset=0;
    std::unique_ptr<uint8_t[]> buffer;
    uint32_t file_length=0;
    std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
    /*	dest contains the ABSOLUTE final position (comprehensive of header and overhead
     * overhead represent the signature and length byte of each sector "jumped"
     * position contains the position as the number of user data byte from the begin of the file  */
    /*if(this->secure_sync()){
        return SEFILE_WRITE_ERROR;
    }*/
#if defined(__linux__) || defined(__APPLE__) // check if current position in SEfile file is the same as the current position in the real file managed by OS
    if(((absOffset=lseek(hTmp->fd, 0, SEEK_CUR)) < 0) || (absOffset != hTmp->log_offset)){
        return SEFILE_SEEK_ERROR;
    }
#elif _WIN32
    if(((absOffset=SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT)) < 0) || (((uint32_t)absOffset) != hTmp->log_offset)){
        return SEFILE_SEEK_ERROR;
    }
#endif
    if(this->get_filesize(&file_length)){ // retrieve the size of the file (only the valid bytes)
        return SEFILE_SEEK_ERROR;
    }
    sectOffset = absOffset % SEFILE_SECTOR_SIZE; // compute the current offset inside the current sector where the file pointer is positioned
    if(whence==SEFILE_BEGIN){
        if(offset<0){			//backward jump not allowed from the file begin
            *position=-1;
            return SEFILE_SEEK_ERROR;
        }else{
        	/* overhead to be added because of seek: overhead in a single sector times how many sectors we should jump forward */
            overhead = (offset / SEFILE_LOGIC_DATA) * SEFILE_SECTOR_OVERHEAD;
            /* actual destination pointer inside SEfile ciphertext: offset requested + overhead + size of header sector */
            dest = offset + overhead + SEFILE_SECTOR_SIZE;
        }
    } else if(whence==SEFILE_CURRENT){
        if(offset<0){			//backward jump
            tmp = (offset + sectOffset); // check if the jump is inside current sector or outside
            if(tmp >= 0){ // inside the sector because sectOffset still bigger than the backward jump
                dest = absOffset + offset;
            } else{ // outside the current sector (in a previous sector, need to add overhead)
                tmp *= (-1); // change sign
                /* overhead to be added because of seek: overhead in a single sector times how many sectors we should jump backward */
                overhead = ((tmp)/(SEFILE_LOGIC_DATA)) * SEFILE_SECTOR_OVERHEAD;
                if((tmp)%SEFILE_LOGIC_DATA){ // add spare overhead lost by previous division (which truncates to integer)
                	overhead+=SEFILE_SECTOR_OVERHEAD;
                }
                /* actual destination inside SEfile ciphertext given by current pointer - overhead - offset */
                dest = absOffset + offset - overhead;
            }
        }else {	// forward jump
        	/* overhead given by overhead in one sector times the number of sectors we should jump forward */
            overhead = ((offset+sectOffset)/SEFILE_LOGIC_DATA) * SEFILE_SECTOR_OVERHEAD;
            dest=absOffset+overhead+offset;
        }
    } else if(whence==SEFILE_END){
        sectOffset = (file_length % SEFILE_LOGIC_DATA); // offset inside the last sector of the file (SEFILE_LOGIC_DATA because file_length is related only to valid bytes inside SEfile)
        absOffset = ((file_length / SEFILE_LOGIC_DATA) + 1) * SEFILE_SECTOR_SIZE + sectOffset; // offset of the pointer of the file, considering entire file, header included
        if(offset<0){			//backward jump
            tmp=(offset + sectOffset);
            if(tmp>=0){			//inside the sector
                dest=absOffset+offset;
            } else{		//outside the current sector (need to add overheads)
                tmp *= (-1);
                overhead=((tmp)/(SEFILE_LOGIC_DATA))* SEFILE_SECTOR_OVERHEAD;
                if((tmp)%SEFILE_LOGIC_DATA){
                	overhead+=SEFILE_SECTOR_OVERHEAD;
                }
                dest=absOffset + offset - overhead;
            }
        } else {				//forward jump
            overhead=((offset+sectOffset)/SEFILE_LOGIC_DATA)*SEFILE_SECTOR_OVERHEAD;
            dest=absOffset+offset+overhead;
        }
    }
    if(dest<SEFILE_SECTOR_SIZE){	//pointer inside the header sector is not allowed
        *position = -1;
        return SEFILE_ILLEGAL_SEEK;
    }
    *position = (dest % SEFILE_SECTOR_SIZE) + (((dest / SEFILE_SECTOR_SIZE) - 1) * SEFILE_LOGIC_DATA); // final position inside the file (only valid bytes)
    buffer_size = (*position) - file_length; // check if current position is ahead of the current valid bytes in the file
    if(buffer_size>0){ 			//if destination exceed the end of the file, empty sectors are inserted at the end of the file to keep the file consistency
        buffer = std::make_unique<uint8_t[]>(buffer_size);
    	if(buffer == nullptr){
            return SEFILE_SEEK_ERROR;
        }
        if((file_length % SEFILE_LOGIC_DATA)){ // there is still space inside the last sector
#if defined(__linux__) || defined(__APPLE__)
            //hTmp->log_offset=lseek(hTmp->fd, ((file_length % SEFILE_LOGIC_DATA) - SEFILE_SECTOR_SIZE), SEEK_END); // wrong, bugfix at next line
            hTmp->log_offset=lseek(hTmp->fd, ((((file_length/SEFILE_LOGIC_DATA)+1/* +1 for header */) * SEFILE_SECTOR_SIZE) + file_length%SEFILE_LOGIC_DATA), SEEK_SET); // move the pointer to the first unused byte that is still usable in the last sector
#elif _WIN32
            //hTmp->log_offset=SetFilePointer(hTmp->fd, ((file_length%SEFILE_LOGIC_DATA)-SEFILE_SECTOR_SIZE), nullptr, FILE_END); // wrong, bugfix at next line
            hTmp->log_offset=SetFilePointer(hTmp->fd, ((((file_length/SEFILE_LOGIC_DATA)+1/* +1 for header */) * SEFILE_SECTOR_SIZE) + file_length%SEFILE_LOGIC_DATA), nullptr, FILE_BEGIN); // move the pointer to the first unused byte that is still usable in the last sector
#endif
        }
        memset(buffer.get(), 0, buffer_size); // we insert empty bytes until the position is reached
        if(this->secure_write(buffer.get(), buffer_size)){
            return SEFILE_SEEK_ERROR;
        }
    } else {
#if defined(__linux__) || defined(__APPLE__)
        tmp=lseek(hTmp->fd, (off_t) dest, SEEK_SET);
        if(( tmp == -1)){
            return SEFILE_SEEK_ERROR;
        }
#elif _WIN32
        tmp = SetFilePointer( hTmp->fd, dest, nullptr, FILE_BEGIN);
        if (((DWORD)tmp) == INVALID_SET_FILE_POINTER){
            return SEFILE_SEEK_ERROR;
        }
#endif
        hTmp->log_offset=tmp;
    }
    return 0;
}

uint16_t SEfile::secure_truncate(uint32_t new_size){
    if((this->l1 == nullptr) || (this->handleptr == nullptr) || (this->IsOpen == false)){
    	return SEFILE_TRUNCATE_ERROR;
    }
	uint32_t fPosition = 0;
    int rOffset = 0, nSector = 0; //New Relative offset & new number of sectors
    std::unique_ptr<uint8_t[]> buffer;
    uint32_t original_size = 0, bytesRead = 0;
    /*if(this->secure_sync()){
        return SEFILE_WRITE_ERROR;
    }*/
    if(this->get_filesize(&original_size)){
        return SEFILE_TRUNCATE_ERROR;
    }
    if(original_size == new_size){
    	return 0; // no need to do anything
    }
    if(original_size < new_size){ // truncate to larger size
    	uint16_t rc = this->secure_seek((new_size-original_size), (int32_t*)&fPosition, SEFILE_END); // secure_seek adds 0s to the end of file if required
        if(rc || (fPosition != (new_size+original_size))){
            return SEFILE_TRUNCATE_ERROR;
        }
    } else {
        rOffset = new_size % SEFILE_LOGIC_DATA; //Relative offset inside a sector
        nSector = (new_size / SEFILE_LOGIC_DATA) + 1; //Number of sectors in a file (including header)
#if defined(__linux__) || defined(__APPLE__)
        this->handleptr->log_offset = lseek(this->handleptr->fd, nSector*SEFILE_SECTOR_SIZE, SEEK_SET); // move file pointer to the destination sector to truncate
        if(this->handleptr->log_offset < 0){ return SEFILE_TRUNCATE_ERROR; }
#elif _WIN32
        this->handleptr->log_offset = SetFilePointer(this->handleptr->fd, nSector*SEFILE_SECTOR_SIZE, nullptr, FILE_BEGIN); // move file pointer to the destination sector to truncate
        if(this->handleptr->log_offset == INVALID_SET_FILE_POINTER){ return SEFILE_TRUNCATE_ERROR; }
#endif
        buffer = std::make_unique<uint8_t[]>(rOffset);
        if(buffer == nullptr){ return SEFILE_TRUNCATE_ERROR; }
        if(this->secure_read(buffer.get(), rOffset, &bytesRead)){ return SEFILE_TRUNCATE_ERROR; } // read the sector at the truncate position
#if defined(__linux__) || defined(__APPLE__)
        this->handleptr->log_offset = lseek(this->handleptr->fd, nSector*SEFILE_SECTOR_SIZE, SEEK_SET); // move file pointer to the destination sector to truncate
        if(this->handleptr->log_offset < 0){ return SEFILE_TRUNCATE_ERROR; }
        if(ftruncate(this->handleptr->fd, nSector*SEFILE_SECTOR_SIZE)){	// truncate
            return SEFILE_TRUNCATE_ERROR;
        }
#elif _WIN32
        this->handleptr->log_offset = SetFilePointer(this->handleptr->fd, nSector*SEFILE_SECTOR_SIZE, nullptr, FILE_BEGIN); // move file pointer to the destination sector to truncate
        if(this->handleptr->log_offset == INVALID_SET_FILE_POINTER){ return SEFILE_TRUNCATE_ERROR; }
        if(!SetEndOfFile(this->handleptr->fd)){	//truncate
            return SEFILE_TRUNCATE_ERROR;
        }
#endif
        if(this->secure_write(buffer.get(), rOffset)){ return SEFILE_TRUNCATE_ERROR; } // write back last sector
    }
    return 0;
}

uint16_t SEfile::secure_close(){
    if(this->handleptr == nullptr){ return 0; }
#if defined(__linux__) || defined(__APPLE__)
	if(close(this->handleptr->fd) == -1 ){
		this->handleptr.reset();
		return SEFILE_CLOSE_HANDLE_ERR;
	}
#elif _WIN32
	if ( CloseHandle(this->handleptr->fd) == 0){
		this->handleptr.reset();
		return SEFILE_CLOSE_HANDLE_ERR;
	}
#endif
	this->handleptr.reset();
	this->IsOpen = false;
    return 0;
}

uint16_t SEfile::crypt_header(void *buff1, void *buff2, size_t datain_len, uint16_t direction){
	if((buff1 == nullptr) || (buff2 == nullptr) || (datain_len /*==*/ < 0) || (this->l1 == nullptr)){ return L0ErrorCodes::Error::SE3_ERR_PARAMS; }
	enum { MAX_DATA_IN = L1Crypto::UpdateSize::DATAIN - (L1Crypto::UpdateRequestOffset::DATA + SEFILE_BLOCK_SIZE) - SEFILE_NONCE_LEN - SEKEY_HDR_LEN };
    uint8_t* sp = (uint8_t*)buff1, *rp = (uint8_t*)buff2;
    uint16_t curr_len = 0;
    uint32_t enc_sess_id = 0;
    size_t curr_chunk = 0;
    //uint16_t flag_reset_auth = datain_len < MAX_DATA_IN ? L1Crypto::UpdateFlags::AUTH : L1Crypto::UpdateFlags::AUTH;
    uint8_t *nonce_pbkdf2 = (uint8_t*)buff1;
    datain_len -= (SEFILE_NONCE_LEN + SEKEY_HDR_LEN);
    curr_chunk = datain_len < MAX_DATA_IN ? datain_len : MAX_DATA_IN;
    try{
    	l1->L1CryptoInit(this->EnvCrypto, CryptoInitialisation::Modes::ECB | direction, this->EnvKeyID, enc_sess_id);
    	l1->L1CryptoUpdate(enc_sess_id, L1Crypto::UpdateFlags::SETNONCE, SEFILE_NONCE_LEN, nonce_pbkdf2, 0, nullptr, nullptr, nullptr);
    } catch(...){
    	return L1Error::Error::SE3_ERR_ACCESS; // no specific reason to return this error, just return non zero
    }
    do {
    	try{
            if(datain_len - curr_chunk){
            	this->l1->L1CryptoUpdate(enc_sess_id, CryptoInitialisation::Modes::ECB | direction, 0, nullptr, curr_chunk, sp + SEFILE_NONCE_LEN + SEKEY_HDR_LEN, &curr_len, rp + SEFILE_NONCE_LEN + SEKEY_HDR_LEN); // + SEKEY_HDR_LEN for SEkey header
            } else{
            	//this->l1->L1CryptoUpdate(enc_sess_id, flag_reset_auth | L1Crypto::UpdateFlags::FINIT, 0, nullptr, curr_chunk, sp + SEFILE_NONCE_LEN + SEKEY_HDR_LEN, &curr_len, rp + SEFILE_NONCE_LEN + SEKEY_HDR_LEN); // + SEKEY_HDR_LEN for SEkey header
            	this->l1->L1CryptoUpdate(enc_sess_id, L1Crypto::UpdateFlags::AUTH | L1Crypto::UpdateFlags::FINIT, 0, nullptr, curr_chunk, sp + SEFILE_NONCE_LEN + SEKEY_HDR_LEN, &curr_len, rp + SEFILE_NONCE_LEN + SEKEY_HDR_LEN); // + SEKEY_HDR_LEN for SEkey header
            }
    	} catch(...){
        	return L1Error::Error::SE3_ERR_ACCESS; // no specific reason to return this error, just return non zero
    	}
        datain_len -= curr_chunk;
        sp += curr_chunk;
        rp += curr_chunk;
        curr_chunk = datain_len < MAX_DATA_IN ? datain_len : MAX_DATA_IN;
    } while (datain_len > 0);
    memcpy(((uint8_t*)buff2)+SEFILE_NONCE_LEN, ((uint8_t*)buff1)+SEFILE_NONCE_LEN, SEKEY_HDR_LEN); // copy sekey header as plaintext
    memcpy(buff2, buff1, SEFILE_NONCE_LEN); // copy the field named "nonce_pbkdf2" as plaintext into the header
    return L1Error::Error::OK;
}

uint16_t SEfile::crypt_sectors(void *buff_decrypt, void *buff_crypt, size_t datain_len, size_t current_offset, uint8_t* nonce_ctr, uint8_t* nonce_pbkdf2){
    if((datain_len /*==*/ < 0) || (buff_crypt == nullptr) || (buff_decrypt == nullptr) || (nonce_ctr == nullptr) || (nonce_pbkdf2 == nullptr) || (this->l1 == nullptr)){
        return L0ErrorCodes::Error::SE3_ERR_PARAMS;
    }
	enum {
        MAX_DATA_IN = L1Crypto::UpdateSize::DATAIN - (L1Crypto::UpdateRequestOffset::DATA + SEFILE_BLOCK_SIZE) - B5_SHA256_DIGEST_SIZE
    };
    uint8_t* sp = (uint8_t*)buff_decrypt, *rp = (uint8_t*)buff_crypt;
    uint16_t curr_len = 0;
    uint32_t enc_sess_id = 0;
    size_t curr_chunk = datain_len < MAX_DATA_IN ? datain_len : MAX_DATA_IN;
    uint8_t nonce_local[16];
    uint16_t flag_reset_auth = datain_len < MAX_DATA_IN ? L1Crypto::UpdateFlags::RESET | L1Crypto::UpdateFlags::AUTH : L1Crypto::UpdateFlags::AUTH;
    try{
    	l1->L1CryptoInit(this->EnvCrypto, CryptoInitialisation::Modes::CTR | CryptoInitialisation::Direction::ENCRYPT, this->EnvKeyID, enc_sess_id);
    	l1->L1CryptoUpdate(enc_sess_id, L1Crypto::UpdateFlags::SETNONCE, SEFILE_NONCE_LEN, nonce_pbkdf2, 0, nullptr, nullptr, nullptr);
    } catch(...){
    	return L1Error::Error::SE3_ERR_ACCESS; // no specific reason to return this error, just return non zero
    }
    memcpy(nonce_local, nonce_ctr, 16);
    compute_blk_offset(current_offset, nonce_local);
    do {
    	try{
        	if (datain_len - curr_chunk){
        		this->l1->L1CryptoUpdate(enc_sess_id, L1Crypto::UpdateFlags::RESET | CryptoInitialisation::Modes::CTR | CryptoInitialisation::Direction::ENCRYPT, SEFILE_BLOCK_SIZE, nonce_local, curr_chunk, sp, &curr_len, rp);
        	} else {
        		this->l1->L1CryptoUpdate(enc_sess_id, flag_reset_auth | L1Crypto::UpdateFlags::FINIT, SEFILE_BLOCK_SIZE, nonce_local, curr_chunk, sp, &curr_len, rp);
        	}
    	} catch(...){
    		return L1Error::Error::SE3_ERR_ACCESS; // no specific reason to return this error, just return non zero
    	}
        compute_blk_offset(((curr_len - B5_SHA256_DIGEST_SIZE) / SEFILE_BLOCK_SIZE), nonce_local);
        datain_len -= curr_chunk;
        sp += curr_chunk;
        rp += curr_chunk;
        curr_chunk = datain_len < MAX_DATA_IN ? datain_len : MAX_DATA_IN;
    } while (datain_len > 0);
    return L1Error::Error::OK;
}

uint16_t SEfile::decrypt_sectors(void *buff_crypt, void *buff_decrypt, size_t datain_len, size_t current_offset, uint8_t* nonce_ctr, uint8_t* nonce_pbkdf2){
    if((datain_len /*==*/ < 0) || (buff_crypt == nullptr) || (buff_decrypt == nullptr) || (nonce_ctr == nullptr) || (nonce_pbkdf2 == nullptr) || (this->l1 == nullptr)){
        return L0ErrorCodes::Error::SE3_ERR_PARAMS;
    }
	enum {
        MAX_DATA_IN = L1Crypto::UpdateSize::DATAIN - (L1Crypto::UpdateRequestOffset::DATA + SEFILE_BLOCK_SIZE) - B5_SHA256_DIGEST_SIZE
    };
    uint8_t* sp = (uint8_t*)buff_crypt, *rp = (uint8_t*)buff_decrypt;
    uint16_t curr_len = 0;
    uint32_t enc_sess_id = 0;
    size_t curr_chunk = datain_len < MAX_DATA_IN ? datain_len : MAX_DATA_IN;
    uint8_t nonce_local[16];
    uint16_t flag_reset_auth = datain_len < MAX_DATA_IN ? L1Crypto::UpdateFlags::RESET | L1Crypto::UpdateFlags::AUTH : L1Crypto::UpdateFlags::AUTH;
    try{
    	l1->L1CryptoInit(this->EnvCrypto, CryptoInitialisation::Modes::CTR | CryptoInitialisation::Direction::DECRYPT, this->EnvKeyID, enc_sess_id);
    	l1->L1CryptoUpdate(enc_sess_id, L1Crypto::UpdateFlags::SETNONCE, SEFILE_NONCE_LEN, nonce_pbkdf2, 0, nullptr, nullptr, nullptr);
    } catch(...){
    	return L1Error::Error::SE3_ERR_ACCESS; // no specific reason to return this error, just return non zero
    }
    memcpy(nonce_local, nonce_ctr, 16);
    compute_blk_offset(current_offset, nonce_local);
    do {
    	try{
    		if(datain_len - curr_chunk){
    			this->l1->L1CryptoUpdate(enc_sess_id, L1Crypto::UpdateFlags::RESET | CryptoInitialisation::Modes::CTR | CryptoInitialisation::Direction::DECRYPT, SEFILE_BLOCK_SIZE, nonce_local, curr_chunk, sp, &curr_len, rp);
			} else {
				this->l1->L1CryptoUpdate(enc_sess_id, flag_reset_auth | L1Crypto::UpdateFlags::FINIT, SEFILE_BLOCK_SIZE, nonce_local, curr_chunk, sp, &curr_len, rp);
			}
    	} catch(...){
    		return L1Error::Error::SE3_ERR_ACCESS; // no specific reason to return this error, just return non zero
    	}
        compute_blk_offset(((curr_len-B5_SHA256_DIGEST_SIZE) / SEFILE_BLOCK_SIZE), nonce_local);
        datain_len -= curr_chunk;
        sp += curr_chunk;
        rp += curr_chunk;
        curr_chunk = datain_len < MAX_DATA_IN ? datain_len : MAX_DATA_IN;
    } while (datain_len > 0);
    return L1Error::Error::OK;
}

uint16_t SEfile::get_filesize(uint32_t * length){
    if((this->handleptr == nullptr) || (this->l1 == nullptr) || (this->IsOpen == false) || (length == nullptr)){
        return SEFILE_FILESIZE_ERROR;
    }
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::DECRYPT); // check if the key is valid for decryption
    if(rc){ return rc; } // return if the key is not valid
	std::unique_ptr<SEFILE_SECTOR> crypt_buffer = std::make_unique<SEFILE_SECTOR>();
    std::unique_ptr<SEFILE_SECTOR> decrypt_buffer = std::make_unique<SEFILE_SECTOR>();
    int32_t total_size=0;
    std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
#if defined(__linux__) || defined(__APPLE__)
    off_t orig_off;
    size_t BytesRead = 0;
#elif _WIN32
    DWORD orig_off;
    DWORD BytesRead = 0;
#endif
    if(crypt_buffer==nullptr || decrypt_buffer==nullptr){
        return SEFILE_FILESIZE_ERROR;
    }
#if defined(__linux__) || defined(__APPLE__)
    orig_off=lseek(hTmp->fd, 0, SEEK_CUR); // save current file offset
    total_size=lseek(hTmp->fd, (-1)*(SEFILE_SECTOR_SIZE), SEEK_END); // move file offset to the beginning of the last sector
    if(orig_off==-1 || total_size==-1){
        return SEFILE_SEEK_ERROR;
    }
    if(!total_size) {
        lseek(hTmp->fd, orig_off, SEEK_SET);
        *length=0;
        return 0;
    }
    if((BytesRead = read(hTmp->fd, crypt_buffer.get(), SEFILE_SECTOR_SIZE))!= SEFILE_SECTOR_SIZE){ // read last sector of the file
        lseek(hTmp->fd, orig_off, SEEK_SET);
        return SEFILE_READ_ERROR;
    }
    if((lseek(hTmp->fd, orig_off, SEEK_SET))==-1){ // restore original file offset position
        return SEFILE_SEEK_ERROR;
    }
#elif _WIN32
    orig_off=SetFilePointer(hTmp->fd, 0, 0, FILE_CURRENT);
    total_size=SetFilePointer(hTmp->fd, (-1)*(SEFILE_SECTOR_SIZE), nullptr, FILE_END);
    if(((uint32_t)orig_off)==INVALID_SET_FILE_POINTER || ((uint32_t)total_size)==INVALID_SET_FILE_POINTER){
        return SEFILE_SEEK_ERROR;
    }
    if(!total_size) {
        SetFilePointer(hTmp->fd, orig_off, nullptr, FILE_BEGIN);
        *length=0;
        return 0;
    }
    if ((ReadFile(hTmp->fd, crypt_buffer.get(), SEFILE_SECTOR_SIZE, &BytesRead, nullptr))==0 || BytesRead!=SEFILE_SECTOR_SIZE){
        SetFilePointer(hTmp->fd, orig_off, nullptr, FILE_BEGIN);
        return SEFILE_READ_ERROR;
    }
    if((SetFilePointer(hTmp->fd, orig_off, nullptr, FILE_BEGIN))==INVALID_SET_FILE_POINTER){
        return SEFILE_SEEK_ERROR;
    }
#endif
    if (this->decrypt_sectors(crypt_buffer.get(), decrypt_buffer.get(), SEFILE_SECTOR_DATA_SIZE, pos_to_cipher_block(total_size), hTmp->nonce_ctr, hTmp->nonce_pbkdf2)){
        return SEFILE_FILESIZE_ERROR;
    }
    if (memcmp(crypt_buffer->signature, decrypt_buffer->signature, B5_SHA256_DIGEST_SIZE)){
        return SEFILE_SIGNATURE_MISMATCH;
    }
    /* the total size of the file (valid file content, excluding header, signature, len, overhead and any other content which is not part of the original plaintext file)
     * is equal to the valid bytes in all sectors but the header and the last sector, + the valid bytes in the last sector. */
    *length=((total_size/SEFILE_SECTOR_SIZE)-1)*SEFILE_LOGIC_DATA + decrypt_buffer->len;
    return 0;
}

uint16_t SEfile::secure_sync(){
    if(this->handleptr == nullptr){
        return SEFILE_SYNC_ERR;
    }
    std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
    uint16_t ret = 0;
#if defined(__linux__) || defined(__APPLE__)
    if(fsync(hTmp->fd)){
        ret = SEFILE_SYNC_ERR;
    }
#elif _WIN32
    if (!FlushFileBuffers(hTmp->fd)){
        ret = SEFILE_SYNC_ERR;
    }
#endif
    return ret;
}



/* SEfile methods that are not related to a specific file */
uint16_t get_secure_context(std::string& filename, std::string *keyid, uint16_t *algo){
	if((keyid == nullptr) || (algo == nullptr)){
		return -1;
	}
	SEfile encryptedfile;
	encryptedfile.l1 = SEcube;
	std::unique_ptr<char[]> p = std::make_unique<char[]>(filename.length()+1);
	memset(p.get(), '\0', filename.length()+1);
	memcpy(p.get(), filename.c_str(), filename.length());
	try{
		override_key_check = true;
		uint16_t rc = encryptedfile.secure_open(p.get(), SEFILE_READ, SEFILE_OPEN);
		override_key_check = false;
		if(rc){	return -1; }
	} catch (...) {
		override_key_check = false;
		return -1;
	}
	keyid->assign("K" + std::to_string(encryptedfile.EnvKeyID));
	*algo = encryptedfile.EnvCrypto;
	if(encryptedfile.secure_close()){
		return -1;
	}
	return 0;
}

uint16_t secure_mkdir(std::string& path, L1 *SEcubeptr, uint32_t key){
	if(SEcubeptr == nullptr){ return SEFILE_MKDIR_ERROR; }
	SEfile dummy(SEcubeptr, key); // dummy SEfile object used simply to check if the specified key can be used for encryption (i.e. active key)
    uint16_t rc = dummy.secure_key_check(CryptoInitialisation::Direction::ENCRYPT); // check if the key is valid
    if(rc){ return rc; } // return if the key is not valid
    char encDirname[MAX_PATHNAME];
    uint32_t enc_len = 0;
#ifdef _WIN32
    DWORD errValue = 0;
#else
    int errValue = 0;
#endif
    memset(encDirname, 0, MAX_PATHNAME*sizeof(char));
    if(crypt_dirname(path, encDirname, &enc_len, SEcubeptr, key) || enc_len > MAX_PATHNAME){ // first 8 bytes are the HEX of the key ID, then AES of folder name
        return SEFILE_MKDIR_ERROR;
    }
#if defined(__linux__)||defined(__APPLE__)
    if(mkdir(encDirname, S_IRWXU | S_IRWXG)){
        if ((errValue = errno) == EEXIST){
            return 0;
        }
        return SEFILE_MKDIR_ERROR;
    }
#elif _WIN32
    if(!CreateDirectory(encDirname, nullptr)){
        if ((errValue = GetLastError()) == ERROR_ALREADY_EXISTS){
            return 0;
        }
        return SEFILE_MKDIR_ERROR;
    }
#endif
    return 0;
}

uint16_t secure_ls(std::string& path, std::vector<std::pair<std::string, std::string>>& list, L1* SEcubeptr){
	if(SEcubeptr == nullptr){ return SEFILE_LS_ERROR; }
    char bufferEnc[MAX_PATHNAME], bufferDec[MAX_PATHNAME];
    uint16_t encoded_length=0;
    char root[MAX_PATHNAME]; // store here the fisrt part of the path sent by the user (i.e. C:\folder\folder\)
	memset(root, '\0', MAX_PATHNAME);
	get_path((char*)path.c_str(), root);
	std::string s1(root); // the name of the file will be appended to s1
#if defined(__linux__) || defined(__APPLE__)
    DIR *hDir=nullptr;
    struct dirent *dDir;
    hDir = opendir(path.c_str());
    while((dDir = readdir(hDir)) != nullptr){
        memset(bufferEnc, 0, MAX_PATHNAME*sizeof(char));
        memset(bufferDec, 0, MAX_PATHNAME*sizeof(char));
    	std::string currname(dDir->d_name);
    	std::string fullpath = s1 + currname; // current file or directory name
        if((!currname.compare(".")) || (!currname.compare(".."))){
            continue;
        }else if(dDir->d_type==DT_DIR){
            if(valid_directory_name(currname)){
            	std::pair<std::string, std::string> p(currname, currname);
            	list.push_back(p);
            	continue;
            }
            if(decrypt_dirname(currname, bufferDec, SEcubeptr) == 0){
            	std::string tmp(bufferDec);
            	std::pair<std::string, std::string> p(currname, tmp);
            	list.push_back(p);
            } else {
            	std::pair<std::string, std::string> p(currname, currname);
            	list.push_back(p);
            	return SEFILE_LS_ERROR;
            }
        }else if(dDir->d_type==DT_REG){
            if(valid_file_name(currname) || decrypt_filename(fullpath, bufferDec, SEcubeptr)){
            	std::pair<std::string, std::string> p(currname, currname);
            	list.push_back(p);
            	continue;
            }
            if(crypto_filename(bufferDec, bufferEnc, &encoded_length)){
            	std::pair<std::string, std::string> p(currname, currname);
            	list.push_back(p);
                return SEFILE_LS_ERROR;
            }
            if(currname.compare(0, std::string::npos, bufferEnc, encoded_length) == 0){
            	std::string tmp(bufferDec);
            	std::pair<std::string, std::string> p(currname, tmp);
            	list.push_back(p);
            } else {
            	std::pair<std::string, std::string> p(currname, currname);
            	list.push_back(p);
                return SEFILE_LS_ERROR;
            }
        }
    }
    closedir(hDir);
#elif _WIN32
    HANDLE hDir;
    WIN32_FIND_DATA dDir;
    hDir=FindFirstFile(path.c_str(), &dDir);
    do{
        memset(bufferEnc, 0, MAX_PATHNAME*sizeof(char));
        memset(bufferDec, 0, MAX_PATHNAME*sizeof(char));
    	std::string currname(dDir.cFileName);
    	std::string fullpath = s1 + currname;
    	if((!currname.compare(".")) || (!currname.compare(".."))){
    		continue;
        }else if(dDir.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY){ // if this is a directory
            if(valid_directory_name(currname)){
            	std::pair<std::string, std::string> p(currname, currname); // name not recognized as generated by SEfile, copy it as it is
            	list.push_back(p);
            	continue;
            }
            if(decrypt_dirname(currname, bufferDec, SEcubeptr) == 0){
            	std::string tmp(bufferDec); // success
            	std::pair<std::string, std::string> p(currname, tmp);
            	list.push_back(p);
            } else { // failure
            	std::pair<std::string, std::string> p(currname, currname);
            	list.push_back(p);
                return SEFILE_LS_ERROR;
            }
        } else { // if this is a file
            if(valid_file_name(currname) || decrypt_filename(fullpath, bufferDec, SEcubeptr)){
            	std::pair<std::string, std::string> p(currname, currname); // this may fail in case of invalid name or encryption key that was destroyed
            	list.push_back(p);
                continue;
            }
            if(crypto_filename(bufferDec, bufferEnc, &encoded_length)){
            	std::pair<std::string, std::string> p(currname, currname); // error
            	list.push_back(p);
                return SEFILE_LS_ERROR;
            }
            if(currname.compare(0, std::string::npos, bufferEnc, encoded_length) == 0){
            	std::string tmp(bufferDec); // success
            	std::pair<std::string, std::string> p(currname, tmp);
            	list.push_back(p);
            } else {
            	std::pair<std::string, std::string> p(currname, currname); // error
            	list.push_back(p);
                return SEFILE_LS_ERROR;
            }
        }
    } while(FindNextFile(hDir, &dDir));
    FindClose(hDir);
#endif
    return 0;
}

uint16_t crypt_dirname(std::string& path, char *encDirname, uint32_t* enc_len, L1* SEcubeptr, uint32_t key){
    if((encDirname == nullptr) || (SEcubeptr == nullptr)){
    	return SEFILE_DIRNAME_ENC_ERROR;
    }
	uint16_t commandError = 0;
    uint8_t *pDir = nullptr;
    int32_t maxLen = 0;
    const char *dirpath = path.c_str();
    char *filename = nullptr;
    std::unique_ptr<uint8_t[]> buffDec;
    std::unique_ptr<uint8_t[]> buffEnc;
    int i=0;
    filename = (char*)strrchr(dirpath, '/');
    if(filename == nullptr){
        filename = (char*)strrchr(dirpath, '\\');
        if(filename == nullptr){
        	filename = (char*)dirpath;
        } else {
        	filename++;
        }
    }else{
        filename++;
    }
    maxLen=(((strlen(filename)/SEFILE_BLOCK_SIZE)+1)*SEFILE_BLOCK_SIZE);
    if(strlen(filename)>(MAX_PATHNAME-2)/2){
        return SEFILE_DIRNAME_ENC_ERROR;
    }
    if((filename-dirpath)>MAX_PATHNAME-maxLen){
        return SEFILE_DIRNAME_ENC_ERROR;
    }
    buffDec = std::make_unique<uint8_t[]>(maxLen+1);
    buffEnc = std::make_unique<uint8_t[]>(maxLen+1);
    if(buffDec==nullptr || buffEnc==nullptr){
        return SEFILE_DIRNAME_ENC_ERROR;
    }
    memcpy(buffDec.get(), filename, strlen(filename));
    if((commandError = encrypt_name(buffDec.get(), buffEnc.get(), maxLen, CryptoInitialisation::Direction::ENCRYPT, SEcubeptr, key))){
        return commandError;
    }
    memcpy(encDirname, dirpath, filename-dirpath);
    pDir = (uint8_t*)(encDirname + (filename - dirpath));
    sprintf((char*)pDir, "%08x", key);
    pDir+=8;
    for(i=0;i<maxLen;i++){
        sprintf((char*)pDir+(i*2), "%02x", (uint8_t)buffEnc[i]);
    }
    if(enc_len != nullptr){
      *enc_len = maxLen + (filename-dirpath);
    }
    pDir[i*2]='\0';
    return 0;
}

uint16_t decrypt_dirname(std::string& path, char *decDirname, L1* SEcubeptr){
    if((decDirname == nullptr) || (SEcubeptr == nullptr)){
    	return SEFILE_DIRNAME_ENC_ERROR;
    }
    uint16_t commandError = 0;
    int32_t maxLen = 0;
    uint32_t key = 0;
    char *filename = nullptr;
    std::unique_ptr<char[]> buffEnc;
    char tmp[9], *pName;
    int32_t i=0;
    memset(tmp, 0, 9);
    const char *dirpath = path.c_str();
    filename = (char*)strrchr(dirpath, '/');
    if(filename == nullptr){
        filename = (char*)strrchr(dirpath, '\\');
        if(filename == nullptr){
        	filename = (char*)dirpath;
        }else{
        	filename++;
        }
    }else{
        filename++;
    }
    maxLen=(strlen(filename)-8);
    buffEnc = std::make_unique<char[]>((maxLen/2)+1);
    if(buffEnc==nullptr){
        return SEFILE_DIRNAME_ENC_ERROR;
    }
    //Conversion of 2 chars at a time from Hex to Integer
    pName=filename+8;
    for(i=0;i<(maxLen/2)+1;i++){
        memcpy(tmp,pName+i*2,2);
        buffEnc[i]=strtol(tmp, nullptr, 16);
    }
    memcpy(tmp, filename, 8); // first 8 chars are the hex-value of the key ID
    key = strtoul(tmp, nullptr, 16); // integer value of the key used to encrypt the name of the directory
    if((commandError = encrypt_name(buffEnc.get(), decDirname, maxLen/2, CryptoInitialisation::Direction::DECRYPT, SEcubeptr, key))){
    	return commandError;
    }
    return 0;
}

uint16_t encrypt_name(void* buff1, void* buff2, size_t size, uint16_t direction, L1 *SEcubeptr, uint32_t key){
    if((buff1 == nullptr) || (buff2 == nullptr) || (SEcubeptr == nullptr)){
    	return L1Error::Error::SE3_ERR_RESOURCE;
    }
	try{
		/* Notice that we do not check if the key can be used for encryption or decryption. This is because the check is done
		 * inside the mkdir API, the ls API does not check anything because we want to always be able to decode the real name
		 * of a folder. */
		if(direction == CryptoInitialisation::Direction::ENCRYPT){
			std::shared_ptr<uint8_t[]> tmp(new uint8_t[size]);
			memcpy(tmp.get(), buff1, size);
			SEcube_ciphertext ciphert;
			SEcubeptr->L1Encrypt(size, tmp, ciphert, L1Algorithms::Algorithms::AES, CryptoInitialisation::Modes::ECB, key);
			memcpy(buff2, ciphert.ciphertext.get(), ciphert.ciphertext_size);
		} else {
			SEcube_ciphertext ciphert;
			ciphert.ciphertext = std::make_unique<uint8_t[]>(size);
			memcpy(ciphert.ciphertext.get(), buff1, size);
			ciphert.ciphertext_size = size;
			ciphert.algorithm = L1Algorithms::Algorithms::AES;
			ciphert.mode = CryptoInitialisation::Modes::ECB;
			ciphert.key_id = key;
			std::shared_ptr<uint8_t[]> tmp;
			size_t tmpsize = 0;
			SEcubeptr->L1Decrypt(ciphert, tmpsize, tmp);
			if(tmp==nullptr){ return L1Error::Error::SE3_ERR_RESOURCE; } // just to avoid compiler warning on next line (arg 2 null when non null expected)
			memcpy(buff2, tmp.get(), tmpsize);
		}
    } catch(...){
    	return L1Error::Error::SE3_ERR_RESOURCE;
    }
    return L1Error::Error::OK;
}

uint16_t decrypt_filename(std::string& path, char *filename, L1 *SEcubeptr){
	// this API is allowed even if the file is still encrypted with a compromised key (because this API is used only by the secure_ls)
	if((filename == nullptr) || (SEcubeptr == nullptr)){
		return SEFILE_FILENAME_DEC_ERROR;
	}
	SEfile currfile;
	currfile.l1 = SEcubeptr;
	currfile.handleptr = std::make_shared<SEFILE_HANDLE>();
	if(currfile.handleptr == nullptr) {	return SEFILE_FILENAME_DEC_ERROR; }
	// open the file (we simply consider it as a binary file)
#if defined(__linux__) || defined(__APPLE__)
	if((currfile.handleptr->fd = open(path.c_str(), O_RDONLY)) == -1 ){
		return SEFILE_FILENAME_DEC_ERROR;
	}
#elif _WIN32
	currfile.handleptr->fd = CreateFile(
				path.c_str(),			              				// file to open
				GENERIC_READ,     								// open for reading and writing
				FILE_SHARE_READ,       							// share
				nullptr,                  							// default security
				OPEN_EXISTING,									// existing file only
				FILE_ATTRIBUTE_NORMAL,							// normal file
				nullptr);                 							// no attr. template
	if (currfile.handleptr->fd == INVALID_HANDLE_VALUE){
		return SEFILE_FILENAME_DEC_ERROR;
	}
#endif
	currfile.IsOpen = true;
	// read the first sector of the file
	int orig_off=0;
#ifdef _WIN32
    DWORD BytesRead;
#endif
	std::unique_ptr<SEFILE_SECTOR> header_buffer = std::make_unique<SEFILE_SECTOR>();
	std::unique_ptr<SEFILE_SECTOR> bufferDec = std::make_unique<SEFILE_SECTOR>();
	if(header_buffer==nullptr || bufferDec==nullptr){
		return SEFILE_FILESIZE_ERROR;
	}
#if defined(__linux__) || defined(__APPLE__)
    orig_off=lseek(currfile.handleptr->fd, 0, SEEK_CUR);
    if(orig_off==-1){
        return SEFILE_FILENAME_DEC_ERROR;
    }
    if (lseek(currfile.handleptr->fd, 0, SEEK_SET)==-1){
        return SEFILE_FILENAME_DEC_ERROR;
    }
    if(read(currfile.handleptr->fd, header_buffer.get(), SEFILE_SECTOR_SIZE)!=SEFILE_SECTOR_SIZE){
        return SEFILE_FILENAME_DEC_ERROR;
    }
    if (lseek(currfile.handleptr->fd, orig_off, SEEK_SET)==-1){
        return SEFILE_FILENAME_DEC_ERROR;
    }
#elif _WIN32
    orig_off = SetFilePointer(currfile.handleptr->fd, 0, 0, FILE_CURRENT); // save current file pointer
    if(((uint32_t)orig_off)==INVALID_SET_FILE_POINTER){
        return SEFILE_FILENAME_DEC_ERROR;
    }
    if(SetFilePointer(currfile.handleptr->fd, 0, nullptr, FILE_BEGIN)==INVALID_SET_FILE_POINTER){ // go to the beginning of the file
        return SEFILE_FILENAME_DEC_ERROR;
    }
    if(ReadFile(currfile.handleptr->fd, header_buffer.get(), SEFILE_SECTOR_SIZE, &BytesRead, nullptr)==0 || BytesRead!=SEFILE_SECTOR_SIZE){ // read the first sector
        return SEFILE_FILENAME_DEC_ERROR;
    }
    if(SetFilePointer(currfile.handleptr->fd, orig_off, nullptr, FILE_BEGIN)==INVALID_SET_FILE_POINTER){ // restore file pointer
        return SEFILE_FILENAME_DEC_ERROR;
    }
#endif
    currfile.EnvKeyID = header_buffer->header.key_header.key_id; // retrieve ID of key used to encrypt this file
    currfile.EnvCrypto = header_buffer->header.key_header.algorithm; // retrieve ID of algorithm used to encrypt this file
    uint8_t length=0;
    if(currfile.crypt_header(header_buffer.get(), bufferDec.get(), SEFILE_SECTOR_DATA_SIZE, CryptoInitialisation::Direction::DECRYPT)){ // decrypt the header
        return SEFILE_FILENAME_DEC_ERROR;
    }
    if (memcmp(header_buffer->signature, bufferDec->signature, B5_SHA256_DIGEST_SIZE)){ // check the signature
        return SEFILE_SIGNATURE_MISMATCH;
    }
    // signature is ok, copy the name of the file
    char temp[MAX_PATHNAME];
    memset(temp, '\0', MAX_PATHNAME);
    length=bufferDec->header.fname_len;
    memcpy(temp, bufferDec->data+sizeof(SEFILE_HEADER), length);
    if(currfile.secure_close()){
        return SEFILE_FILENAME_DEC_ERROR;
    }
    // check if the file was created during re-encryption (ends with .reencryptedsefile)
    char buff1[MAX_PATHNAME], buff2[MAX_PATHNAME];
    memset(buff1, 0, MAX_PATHNAME*sizeof(char));
    memset(buff2, 0, MAX_PATHNAME*sizeof(char));
    uint16_t encoded_length=0;
    std::string cleartext(temp);
    cleartext.append(".reencryptedsefile");
    if(crypto_filename((char*)cleartext.c_str(), buff1, &encoded_length)){
    	return SEFILE_FILENAME_DEC_ERROR;
    }
    get_filename((char*)path.c_str(), buff2);
    // copy the name of the file to the pointer as output
    if(strcmp(buff1, buff2) == 0){
    	memcpy(filename, cleartext.c_str(), cleartext.length());
    	filename[cleartext.length()] = '\0';
    } else {
    	memcpy(filename, temp, strlen(temp));
    	filename[strlen(temp)] = '\0';
    }
    return 0;
}

uint16_t crypto_filename(char *path, char *enc_name, uint16_t *encoded_length){
    if((path == nullptr) || (enc_name == nullptr)){
    	return SEFILE_FILENAME_ENC_ERROR;
    }
	uint16_t commandError=0;
    uint8_t orig_filename[MAX_PATHNAME];
    uint8_t bufferName[MAX_PATHNAME], SHAName[MAX_PATHNAME];
    uint8_t finalPath[MAX_PATHNAME];
    uint16_t lorig=0, i=0;
    B5_tSha256Ctx ctx;
    int finalLen = 0;
    memset(orig_filename, 0, MAX_PATHNAME*sizeof(char));
    memset(bufferName, 0, MAX_PATHNAME*sizeof(char));
    memset(SHAName, 0, MAX_PATHNAME*sizeof(char));
    memset(finalPath, 0, MAX_PATHNAME*sizeof(char));
    get_filename(path, (char*)orig_filename);
    get_path(path, (char*)finalPath);
    lorig=strlen((const char*)orig_filename);
    if((commandError = B5_Sha256_Init(&ctx))){
        return commandError;
    }
    if((commandError = B5_Sha256_Update(&ctx, (uint8_t *)orig_filename, lorig*sizeof(uint8_t)))){
        return commandError;
    }
    if((commandError = B5_Sha256_Finit(&ctx, (uint8_t *)bufferName))){
        return commandError;
    }
    if (strlen((const char*)finalPath)>MAX_PATHNAME-B5_SHA256_DIGEST_SIZE){
        return SEFILE_FILENAME_ENC_ERROR;
    }
    for(i=0; i<B5_SHA256_DIGEST_SIZE; i++){
        sprintf((char*)&(SHAName[i*2]), "%02x", (uint8_t)bufferName[i]);
    }
    finalLen=strlen((const char*)finalPath);
    if(encoded_length != nullptr)
        *encoded_length=i*2+finalLen;
    if(finalPath[0]){
        memcpy(enc_name, finalPath, finalLen);
	}
    memcpy(enc_name+finalLen, SHAName, i*2);
    return 0;
}

uint16_t secure_getfilesize(char *path, uint32_t *position, L1 *SEcubeptr){
    if((path == nullptr) || (position == nullptr) || (SEcubeptr == nullptr)){
    	return SEFILE_FILESIZE_ERROR;
    }
	uint16_t ret = L1Error::Error::OK;
    SEfile currfile;
    currfile.l1 = SEcubeptr;
    if (currfile.secure_open(path, SEFILE_READ, SEFILE_OPEN)){ // key and algo are automatically set
        return SEFILE_FILESIZE_ERROR;
    }
    ret = currfile.get_filesize(position);
    if(currfile.secure_close()){
        return SEFILE_FILESIZE_ERROR;
    }
    return ret;
}

uint16_t secure_recrypt(std::string path, uint32_t key, L1 *SEcubeptr){
	char enc_filename_old[MAX_PATHNAME], enc_filename_new[MAX_PATHNAME];
	try{
		if(SEcubeptr == nullptr){ return SEFILE_RECRYPT_ERROR; }
		int32_t pos;
		uint32_t bytesread, bytesleft, oldsize, toread;
		uint8_t buffer[1024];
		// generate the name (both cleartext and SHA-256) of the new file to be created, including the absolute path (if any)
		std::string newfilename(path + ".reencryptedsefile");
		memset(enc_filename_old, 0, MAX_PATHNAME*sizeof(char)); // SHA-256 of old file name
		memset(enc_filename_new, 0, MAX_PATHNAME*sizeof(char));
		if(crypto_filename((char*)path.c_str(), enc_filename_old, nullptr) != 0){ return SEFILE_RECRYPT_ERROR; }
		if(crypto_filename((char*)newfilename.c_str(), enc_filename_new, nullptr) != 0){ return SEFILE_RECRYPT_ERROR; }
		// open the old file and the new file
		SEfile oldfile(SEcubeptr);
		SEfile newfile(SEcubeptr, key, L1Algorithms::Algorithms::AES_HMACSHA256);
		override_key_check = true;
		if(secure_getfilesize((char*)path.c_str(), &oldsize, SEcubeptr) ||
		   oldfile.secure_open((char*)path.c_str(), SEFILE_READ, SEFILE_OPEN) ||
		   newfile.secure_open((char*)newfilename.c_str(), SEFILE_WRITE, SEFILE_NEWFILE) ||
		   oldfile.secure_seek(0, &pos, SEFILE_BEGIN)){
			override_key_check = false;
			return SEFILE_RECRYPT_ERROR;
		}
		// copy the old file into the new file (basically the content of the old file will be encrypted in a new file, with a new key)
		bytesleft = oldsize;
		while(bytesleft > 0){
			if(bytesleft > 1024){
				toread = 1024;
			} else {
				toread = bytesleft;
			}
			memset(buffer, 0, 1024);
			bytesread = 0;
			if(oldfile.secure_read(buffer, toread, &bytesread) == 0){
				if(bytesread > 0){
					if(newfile.secure_write(buffer, bytesread)){
						remove(enc_filename_new); // error, remove new file
						override_key_check = false;
						return SEFILE_RECRYPT_ERROR;
					} else {
						bytesleft -= bytesread; // decrement bytes to be re-encrypted
					}
				}
			} else {
				remove(enc_filename_new); // error, remove new file
				override_key_check = false;
				return SEFILE_RECRYPT_ERROR;
			}
		}
		override_key_check = false;
		oldfile.secure_close();
		newfile.secure_close();
		//remove(enc_filename_old); // delete old file
		//rename(enc_filename_new, enc_filename_old); // rename re-encrypted file
		override_key_check = false;
		return 0;
	} catch (...) {
		override_key_check = false;
		remove(enc_filename_new); // error, remove new file
		throw;
	}
}



/* SEfile methods used for internal purposes */
void compute_blk_offset(size_t current_offset, uint8_t* nonce){
    if(nonce == nullptr){
    	return;
    }
	uint8_t i = 15, old_v;
    uint16_t cb;
    do{
        old_v = nonce[i];
        nonce[i] += (uint8_t)(current_offset & 0xFF);
        current_offset = (current_offset>>8);
        cb = nonce[i] < old_v;
        if (cb) nonce[i - 1]++;
    } while (i-- && current_offset > 0);
}

uint16_t valid_directory_name(std::string& name){
    int len = name.length();
    if(len < 24){ // a valid name is long at least 24 chars (8 for key ID and at least 16 for encrypted name)
    	return SEFILE_NAME_NOT_VALID;
    }
	for(int i=0; i<len; i++){ // check if all chars are hexadecimal
        if(!isalnum(name.at(i))){
        	return SEFILE_NAME_NOT_VALID;
        }
        if(((name.at(i) >= '0') && (name.at(i) <= '9')) || ((name.at(i) >= 'a') && (name.at(i) <= 'f'))) {
        	continue;
        }
        return SEFILE_NAME_NOT_VALID; // alphanumeric but not in the hex range...
    }
    return 0;
}

uint16_t valid_file_name(std::string& name){
	int len = name.length();
	if(len != 64){ // a valid name is long exactly 64 chars (result of SHA-256)
		return SEFILE_NAME_NOT_VALID;
	}
    for(int i=0; i<len; i++){
        if(!isalnum(name.at(i))){
        	return SEFILE_NAME_NOT_VALID;
        }
        if(((name.at(i) >= '0') && (name.at(i) <= '9')) || ((name.at(i) >= 'a') && (name.at(i) <= 'f'))) {
        	continue;
        }
        return SEFILE_NAME_NOT_VALID;
    }
    return 0;
}

size_t pos_to_cipher_block(size_t current_position){
	return ((current_position / SEFILE_SECTOR_SIZE) - 1) * (SEFILE_SECTOR_DATA_SIZE / SEFILE_BLOCK_SIZE);
}

void get_path(char *full_path, char *path){
    if((full_path == nullptr) || (path == nullptr)){
    	return;
    }
	char *p_name;
    p_name=strrchr(full_path, '/');
    if(p_name==nullptr){
        p_name=strrchr(full_path, '\\');
        if(p_name==nullptr) {
            path[0]='\0';
        }
        else{
            memcpy(path, full_path, p_name-full_path+1);
            path[p_name-full_path+1]='\0';
        }
    }else{
        memcpy(path, full_path, p_name-full_path+1);
        path[p_name-full_path+1]='\0';
    }
}

void get_filename(char *path, char *file_name){
    if((file_name == nullptr) || (path == nullptr)){
    	return;
    }
	char *f_name;
    f_name=strrchr(path, '/');
    if(f_name==nullptr){
        f_name=strrchr(path, '\\');
        if(f_name==nullptr){
            strcpy(file_name, path);
        }
        else{
            strcpy(file_name, f_name+1);
        }
    }else{
        strcpy(file_name, f_name+1);
    }
}
