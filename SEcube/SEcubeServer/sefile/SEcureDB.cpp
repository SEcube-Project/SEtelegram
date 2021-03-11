/**
  ******************************************************************************
  * File Name          : SEcureDB.cpp
  * Description        : Implementation of encrypted SQLite database (with SEfile).
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

#include "SEcureDB.h"
#include "environment.h"
#include <algorithm>

std::vector<std::unique_ptr<SEfile>> databases; // see environment.h

SEFILE_SQL_SECTOR::SEFILE_SQL_SECTOR(){
	this->len = 0;
	memset(this->overhead, 0, SEFILE_SQL_OVERHEAD_LEN);
	memset(this->padding, 0, SEFILE_SQL_PADDING_LEN);
	memset(this->signature, 0, 32);
	memset(this->data, 0, SEFILE_SQL_LOGIC_DATA);
}

/* Functions inherited from SEfile, they have been modified in order to exploit the custom sector for SQLite. */
uint16_t SEfile::securedb_secure_create(char *path_, std::shared_ptr<SEFILE_HANDLE> hFile, int mode){
    if((path_ == nullptr) || (this->l1 == nullptr) || (hFile == nullptr) || (this->EnvKeyID == 0) || (this->EnvCrypto != L1Algorithms::Algorithms::AES_HMACSHA256)){
    	return SEFILE_CREATE_ERROR;
    }
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::ENCRYPT); // check if the key is valid
    if(rc){ return rc; } // return if the key is not valid
    char path[MAX_PATHNAME]; /* search for leading backslashes in path and remove them */
    memset(path, '\0', MAX_PATHNAME*sizeof(char));
    std::string tmp_path(path_);
    while(tmp_path.at(0)=='\\'){ tmp_path.erase(tmp_path.begin()); }
    for(unsigned int n=0; n<tmp_path.length(); n++){ path[n] = tmp_path.at(n); }
	uint16_t commandError=0;
    char enc_filename[MAX_PATHNAME], *filename=nullptr;
    uint16_t lenc=0;
    std::unique_ptr<SEFILE_SQL_SECTOR> buff = std::make_unique<SEFILE_SQL_SECTOR>();
    std::unique_ptr<SEFILE_SQL_SECTOR> buffEnc = std::make_unique<SEFILE_SQL_SECTOR>();
    size_t random_padding = 0;
    uint8_t *padding_ptr = nullptr;
#ifdef _WIN32
    DWORD nBytesWritten = 0;
#endif
    if(buff==nullptr || buffEnc==nullptr){
        return SEFILE_CREATE_ERROR;
    }
    memset(enc_filename, 0, MAX_PATHNAME*sizeof(char));
    if(crypto_filename(path, enc_filename, ((uint16_t*)&lenc))){
        return SEFILE_CREATE_ERROR;
    }
    /*if(hTmp==nullptr){
        return SEFILE_CREATE_ERROR;
    }*/
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
    std::string tmps(".reencryptedsefile"); // remove this substring from path (so this is set in the header)
    if((tmp_path.length() > tmps.length()) && (tmp_path.compare(tmp_path.length() - tmps.length(), tmps.length(), tmps) == 0)){
    	// notice that path and tmp_path are the same
    	int start = strlen(path) - tmps.length();
    	memset(path+start, '\0', tmps.length());
    }
    /* end */
    filename=strrchr(path, '/'); // search forward slash from the end
    if(filename==nullptr){
        filename=strrchr(path, '\\'); // if not found, search backslash from the end
        if(filename==nullptr){
        	filename=path; // if no slashes, then the name of the file is the path
        }
        else {
        	filename++; // first char after backslash
        }
    }else{
        filename++; // first char after forward slash
    }
    buff->header.fname_len=strlen(filename);
    buff->len=0;
    L0Support::Se3Rand(SEFILE_SQL_OVERHEAD_LEN, buff->overhead);
    L0Support::Se3Rand(SEFILE_SQL_PADDING_LEN, buff->padding);
    //copy filename in header
    memcpy(buff->data+sizeof(SEFILE_HEADER), filename, buff->header.fname_len);
    padding_ptr = (buff->data + sizeof(SEFILE_HEADER) + buff->header.fname_len);
    random_padding = (buff->data+SEFILE_SQL_LOGIC_DATA) - padding_ptr;
    L0Support::Se3Rand(random_padding, padding_ptr);
    if (this->crypt_header(buff.get(), buffEnc.get(), SEFILE_SQL_SECTOR_DATA_SIZE, CryptoInitialisation::Direction::ENCRYPT)){
    	return SEFILE_CREATE_ERROR;
    }
#if defined(__linux__) || defined(__APPLE__)
    if(write(hFile->fd, buffEnc.get(), sizeof(SEFILE_SQL_SECTOR)) != sizeof(SEFILE_SQL_SECTOR) ){
    	this->secure_close();
    	return SEFILE_CREATE_ERROR;
    }
#elif _WIN32
    if (WriteFile(hFile->fd, buffEnc.get(), sizeof(SEFILE_SQL_SECTOR), &nBytesWritten, nullptr) == FALSE){
    	return SEFILE_CREATE_ERROR;
    }
#endif
    /* create new header sector end **************************************/
    /* move pointer after the first sector */
#if defined(__linux__) || defined(__APPLE__)
    hFile->log_offset=lseek(hFile->fd, 0, SEEK_CUR);
#elif _WIN32
    hFile->log_offset=SetFilePointer(hFile->fd, 0, nullptr, FILE_CURRENT);
#endif
    if(commandError == 0){
    	this->IsOpen = true;
    }
    return commandError;
}

uint16_t SEfile::securedb_secure_open(char *path_, int32_t mode, int32_t creation){
    if((path_ == nullptr) || (this->l1 == nullptr)){
    	return SEFILE_OPEN_ERROR; }
    /* search for leading backslashes in path and remove them (may be caused by SQLite custom VFS) */
    char path[MAX_PATHNAME];
    memset(path, '\0', MAX_PATHNAME*sizeof(char));
    std::string tmp_path(path_);
    while(tmp_path.at(0)=='\\'){ tmp_path.erase(tmp_path.begin()); }
    for(unsigned int n=0; n<tmp_path.length(); n++){ path[n] = tmp_path.at(n); }
    /* end */
	uint16_t commandError=0;
    char enc_filename[MAX_PATHNAME];
    uint16_t lenc=0;
    std::shared_ptr<SEFILE_HANDLE> hTmp = std::make_shared<SEFILE_HANDLE>();
    SEFILE_SQL_SECTOR buffEnc, buffDec;
	#ifdef _WIN32
    DWORD nBytesRead = 0;
	#endif
    memset(enc_filename, 0, MAX_PATHNAME*sizeof(char));
    if(creation==(SEFILE_NEWFILE)){ // in this case the file must be created
        if((commandError = this->securedb_secure_create(path, hTmp, mode)) != 0){
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
        return SEFILE_OPEN_ERROR;
    }
	#endif
    /* open phase end */
#if defined(__linux__) || defined(__APPLE__)
    if (read(hTmp->fd, &buffEnc, sizeof(SEFILE_SQL_SECTOR)) != sizeof(SEFILE_SQL_SECTOR)){
    	this->handleptr = std::move(hTmp);
    	this->secure_close();
    	return SEFILE_OPEN_ERROR;
    }
    hTmp->log_offset = lseek(hTmp->fd, 0, SEEK_CUR);
#elif _WIN32
    if (ReadFile(hTmp->fd, &buffEnc, sizeof(SEFILE_SQL_SECTOR), &nBytesRead, nullptr) == FALSE && nBytesRead != sizeof(SEFILE_SQL_SECTOR)){
    	this->handleptr = std::move(hTmp);
    	this->secure_close();
    	return SEFILE_OPEN_ERROR;
    }
    hTmp->log_offset = SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT);
#endif
    this->EnvKeyID = buffEnc.header.key_header.key_id; // assign key to be used (must be done before crypt_header is called)
    this->EnvCrypto = buffEnc.header.key_header.algorithm;
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::DECRYPT); // check if the key is valid
    if(rc){
    	this->handleptr = std::move(hTmp);
    	this->secure_close();
    	return rc;
    } // return if the key is not valid
    if(this->crypt_header(&buffEnc, &buffDec, SEFILE_SQL_SECTOR_DATA_SIZE, CryptoInitialisation::Direction::DECRYPT)){
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

uint16_t SEfile::securedb_secure_write(uint8_t * dataIn, uint32_t dataIn_len){
    if((dataIn == nullptr) || (this->IsOpen == false) || (this->l1 == nullptr)){
    	return SEFILE_WRITE_ERROR;
    }
    if(dataIn_len==0){ return 0; }
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::ENCRYPT); // check if the key is valid
    if(rc){ return rc; } // return if the key is not valid
	std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
    int32_t absOffset=0, sectOffset=0;
    std::unique_ptr<SEFILE_SQL_SECTOR> cryptBuff = std::make_unique<SEFILE_SQL_SECTOR>();
    std::unique_ptr<SEFILE_SQL_SECTOR> decryptBuff = std::make_unique<SEFILE_SQL_SECTOR>();
    int length = 0;
    size_t current_position = SEFILE_SQL_SECTOR_SIZE;
    size_t random_padding = 0;
    uint8_t *padding_ptr = nullptr;
#if defined(__linux__) || defined(__APPLE__)
    int nBytesRead=0;
#elif _WIN32
    DWORD nBytesWritten=0, nBytesRead=0;
#endif
//        if (this->sql_secure_sync(hFile)){
//            return SEFILE_WRITE_ERROR;
//        }
    if(cryptBuff==nullptr || decryptBuff==nullptr){
        return SEFILE_WRITE_ERROR;
    }
    //move the pointer to the begin of the sector
#if defined(__linux__) || defined(__APPLE__)
    if((absOffset=lseek(hTmp->fd, 0, SEEK_CUR))<0 || ((uint32_t)absOffset)!=hTmp->log_offset){
        return SEFILE_WRITE_ERROR;
    }
    current_position = lseek(hTmp->fd,((int32_t)(absOffset/SEFILE_SQL_SECTOR_SIZE))*SEFILE_SQL_SECTOR_SIZE, SEEK_SET);
#elif _WIN32
    if((absOffset=SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT))<0 || ((uint32_t)absOffset)!=hTmp->log_offset){
        return SEFILE_WRITE_ERROR;
    }
    current_position = SetFilePointer(hTmp->fd,((int32_t)(absOffset/SEFILE_SQL_SECTOR_SIZE))*SEFILE_SQL_SECTOR_SIZE, nullptr, FILE_BEGIN);
#endif
    //save the relative position inside the sector
    sectOffset=absOffset % SEFILE_SQL_SECTOR_SIZE;
    //read the whole sector and move back the pointer
#if defined(__linux__) || defined(__APPLE__)
    nBytesRead = read(hTmp->fd, cryptBuff.get(), SEFILE_SQL_SECTOR_SIZE);
#elif _WIN32
    ReadFile(hTmp->fd, cryptBuff.get(), SEFILE_SQL_SECTOR_SIZE, &nBytesRead, nullptr);
#endif
    if(nBytesRead>0){
        if (this->decrypt_sectors(cryptBuff.get(), decryptBuff.get(), SEFILE_SQL_SECTOR_DATA_SIZE, securedb_pos_to_cipher_block(current_position), hTmp->nonce_ctr, hTmp->nonce_pbkdf2)){
            return SEFILE_WRITE_ERROR;
        }
        //sector integrity check
        if (memcmp(cryptBuff->signature, decryptBuff->signature, B5_SHA256_DIGEST_SIZE)){
            return SEFILE_SIGNATURE_MISMATCH;
        }
		#if defined(__linux__) || defined(__APPLE__)
		lseek(hTmp->fd, (-1)*SEFILE_SQL_SECTOR_SIZE, SEEK_CUR);
		#elif _WIN32
		SetFilePointer(hTmp->fd, (-1)*SEFILE_SQL_SECTOR_SIZE, nullptr, FILE_CURRENT);
		#endif
    }else{
        //sector empty
        decryptBuff->len=0;
    }
    do{
        //fill the sector with input data until datain are over or the sector is full
        length = dataIn_len < (uint32_t)SEFILE_SQL_LOGIC_DATA-sectOffset? dataIn_len : SEFILE_SQL_LOGIC_DATA-sectOffset;
        memcpy(decryptBuff->data+sectOffset, dataIn, length);
        //update sector data length if needed
        if( (length + (sectOffset)) > decryptBuff->len){
            decryptBuff->len = length + sectOffset;
        }
        /*Padding must be random! (known plaintext attack)*/
        padding_ptr = decryptBuff->data + decryptBuff->len; // matteo
        random_padding = decryptBuff->data + SEFILE_SQL_LOGIC_DATA - padding_ptr;
        L0Support::Se3Rand(random_padding, padding_ptr);
        //encrypt sector
        L0Support::Se3Rand(SEFILE_SQL_OVERHEAD_LEN, cryptBuff->overhead);
        L0Support::Se3Rand(SEFILE_SQL_PADDING_LEN, decryptBuff->padding);
        if (this->crypt_sectors(decryptBuff.get(), cryptBuff.get(), SEFILE_SQL_SECTOR_DATA_SIZE, securedb_pos_to_cipher_block(current_position), hTmp->nonce_ctr, hTmp->nonce_pbkdf2)){
            return SEFILE_WRITE_ERROR;
        }
        /* writeback sector into file phase start */
		#if defined(__linux__) || defined (__APPLE__)
        if(write(hTmp->fd, cryptBuff.get(), SEFILE_SQL_SECTOR_SIZE) != SEFILE_SQL_SECTOR_SIZE){
            return SEFILE_WRITE_ERROR;
        }
		#elif _WIN32
        if (WriteFile(hTmp->fd, cryptBuff.get(), SEFILE_SQL_SECTOR_SIZE, &nBytesWritten, nullptr) == FALSE){
            return SEFILE_WRITE_ERROR;
        }
        if(nBytesWritten != (DWORD) SEFILE_SQL_SECTOR_SIZE){
            return SEFILE_WRITE_ERROR;
        }
		#endif
        /* writeback sector into file phase end */
        current_position += SEFILE_SQL_SECTOR_SIZE;
        dataIn_len-=length;
        dataIn+=length;
        sectOffset=(sectOffset+length)%(SEFILE_SQL_LOGIC_DATA);
        decryptBuff->len=0;
    } while(dataIn_len>0); //cycles unless all dataIn are processed
    //move the pointer inside the last sector written
    if(sectOffset!=0){
#if defined(__linux__) || defined(__APPLE__)
        hTmp->log_offset=lseek(hTmp->fd, (sectOffset - SEFILE_SQL_SECTOR_SIZE), SEEK_CUR);
#elif _WIN32
        hTmp->log_offset = SetFilePointer(hTmp->fd, (LONG)(sectOffset - SEFILE_SQL_SECTOR_SIZE), nullptr, FILE_CURRENT);
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

uint16_t SEfile::securedb_secure_read(uint8_t * dataOut, uint32_t dataOut_len, uint32_t * bytesRead){
    if((this->IsOpen == false) || (this->l1 == nullptr) || (this->IsOpen == false) || (dataOut == nullptr)){
    	return SEFILE_READ_ERROR;
    }
    if((dataOut_len == 0) && (bytesRead != nullptr)){
    	*bytesRead = 0;
    	return 0;
    }
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::DECRYPT); // check if the key is valid
    if(rc){ return rc; } // return if the key is not valid
    std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
    int32_t absOffset=0, sectOffset=0;
    uint32_t dataRead=0;
    std::unique_ptr<SEFILE_SQL_SECTOR> cryptBuff = std::make_unique<SEFILE_SQL_SECTOR>();
    std::unique_ptr<SEFILE_SQL_SECTOR> decryptBuff = std::make_unique<SEFILE_SQL_SECTOR>();
    int length = 0;
    size_t current_position = SEFILE_SQL_SECTOR_SIZE;
    int32_t data_remaining = 0;
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
    if((absOffset=lseek(hTmp->fd, 0, SEEK_CUR))<0 || ((uint32_t)absOffset) !=hTmp->log_offset){
        return SEFILE_READ_ERROR;
    }
    current_position = lseek(hTmp->fd,((int32_t)(absOffset/SEFILE_SQL_SECTOR_SIZE))*SEFILE_SQL_SECTOR_SIZE, SEEK_SET);
#elif _WIN32
    if((absOffset=SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT))<0 || ((uint32_t)absOffset) !=hTmp->log_offset){
        return SEFILE_READ_ERROR;
    }
    current_position = SetFilePointer(hTmp->fd, ((int32_t)(absOffset / SEFILE_SQL_SECTOR_SIZE))*SEFILE_SQL_SECTOR_SIZE, nullptr, FILE_BEGIN);
#endif
    //save the relative position inside the sector
    sectOffset = absOffset % SEFILE_SQL_SECTOR_SIZE;
    do{
        //read the whole sector
#if defined(__linux__) || defined(__APPLE__)
        nBytesRead=read(hTmp->fd, cryptBuff.get(), SEFILE_SQL_SECTOR_SIZE);
#elif _WIN32
        ReadFile(hTmp->fd, cryptBuff.get(), SEFILE_SQL_SECTOR_SIZE, &nBytesRead, nullptr);
#endif
        if(nBytesRead>0){
            if (this->decrypt_sectors(cryptBuff.get(), decryptBuff.get(), SEFILE_SQL_SECTOR_DATA_SIZE, securedb_pos_to_cipher_block(current_position), hTmp->nonce_ctr, hTmp->nonce_pbkdf2)){
                return SEFILE_READ_ERROR;
            }
            //sector integrity check
            if (memcmp(cryptBuff->signature, decryptBuff->signature, B5_SHA256_DIGEST_SIZE)){
                return SEFILE_SIGNATURE_MISMATCH;
            }
        }else{
            break;
        }
        data_remaining = (decryptBuff->len) - sectOffset; //remaining data in THIS sector
        length = dataOut_len < (uint32_t)SEFILE_SQL_LOGIC_DATA-sectOffset? dataOut_len : SEFILE_SQL_LOGIC_DATA-sectOffset;
        if(data_remaining<length){
            length = data_remaining;
        }
        memcpy(dataOut+dataRead, decryptBuff->data+sectOffset, length);
        current_position += SEFILE_SQL_SECTOR_SIZE;
        dataOut_len-=length;
        dataRead+=length;
        sectOffset=(sectOffset+length) % SEFILE_SQL_LOGIC_DATA;
    }while(dataOut_len>0); //cycles unless all data requested are read
    //move the pointer inside the last sector read
    if(sectOffset!=0){
#if defined(__linux__) || defined(__APPLE__)
        hTmp->log_offset=lseek(hTmp->fd, (-1)*(SEFILE_SQL_SECTOR_SIZE-sectOffset), SEEK_CUR); //todo check sectoffset
#elif _WIN32
        hTmp->log_offset=SetFilePointer(hTmp->fd,(-1)*(SEFILE_SQL_SECTOR_SIZE-sectOffset), nullptr, FILE_CURRENT);
#endif
    }else{
#if defined(__linux__) || defined(__APPLE__)
        hTmp->log_offset=lseek(hTmp->fd, 0, SEEK_CUR); //todo check sectoffset
#elif _WIN32
        hTmp->log_offset=SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT);
#endif
    }
	if (bytesRead != nullptr){ *bytesRead=dataRead; }
    return 0;
}

uint16_t SEfile::securedb_secure_seek(int32_t offset, int32_t *position, uint8_t whence){
    if((position==nullptr) || (this->IsOpen == false) || (this->l1 == nullptr)){
    	return SEFILE_SEEK_ERROR;
    }
	int32_t dest=0, tmp=0, buffer_size=0;
    int32_t overhead=0, absOffset=0, sectOffset=0;
    std::unique_ptr<uint8_t[]> buffer;
    uint32_t file_length=0;
    std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
    /*	dest contains the ABSOLUTE final position (comprehensive of header and overhead
     * overhead represent the signature and length byte of each sector "jumped"
     * position contains the position as the number of user data byte from the begin of the file
     */
    //    if (secure_sync(hFile)){
    //        return SEFILE_WRITE_ERROR;
    //    }
#if defined(__linux__) || defined(__APPLE__)
    if((absOffset=lseek(hTmp->fd, 0, SEEK_CUR))<0 || ((uint32_t)absOffset) != hTmp->log_offset){
        return SEFILE_SEEK_ERROR;
    }
#elif _WIN32
    if((absOffset=SetFilePointer(hTmp->fd, 0, nullptr, FILE_CURRENT))<0 || ((uint32_t)absOffset) != hTmp->log_offset){
        return SEFILE_SEEK_ERROR;
    }
#endif
    if(this->securedb_get_filesize(&file_length)){
        return SEFILE_SEEK_ERROR;
    }
    sectOffset = absOffset % SEFILE_SQL_SECTOR_SIZE;
    if(whence==SEFILE_BEGIN){
        if(offset<0){			//backward jump not allowed from the file begin
            *position=-1;
            return SEFILE_SEEK_ERROR;
        }else{
            overhead=(offset/SEFILE_SQL_LOGIC_DATA)*SEFILE_SQL_SECTOR_OVERHEAD;
            dest=offset+overhead+SEFILE_SQL_SECTOR_SIZE;
        }
    } else if(whence==SEFILE_CURRENT){
        if(offset < 0){			//backward jump
            tmp = (offset + sectOffset);
            if(tmp >= 0){			//inside the sector
                dest = absOffset + offset;
            } else{				//outside the current sector (need to add overheads)
                tmp *= (-1);
                overhead=((tmp)/(SEFILE_SQL_LOGIC_DATA))* SEFILE_SQL_SECTOR_OVERHEAD;
                if((tmp)%SEFILE_SQL_LOGIC_DATA){
                	overhead+=SEFILE_SQL_SECTOR_OVERHEAD;
                }
                dest=absOffset + offset - overhead;
            }
        }else {					//forward jump
            overhead=((offset+sectOffset)/SEFILE_SQL_LOGIC_DATA)*SEFILE_SQL_SECTOR_OVERHEAD;
            dest=absOffset+overhead+offset;
        }
    } else if(whence==SEFILE_END){
        sectOffset=(file_length%SEFILE_SQL_LOGIC_DATA);
        absOffset=((file_length/SEFILE_SQL_LOGIC_DATA)+1)*SEFILE_SQL_SECTOR_SIZE+sectOffset;
        if(offset<0){			//backward jump
            tmp=(offset + sectOffset);
            if(tmp>=0){			//inside the sector
                dest=absOffset+offset;
            } else{		//outside the current sector (need to add overheads)
                tmp *= (-1);
                overhead=((tmp)/(SEFILE_SQL_LOGIC_DATA))* SEFILE_SQL_SECTOR_OVERHEAD;
                if((tmp)%SEFILE_SQL_LOGIC_DATA){
                	overhead+=SEFILE_SQL_SECTOR_OVERHEAD;
                }
                dest=absOffset + offset - overhead;
            }
        } else {				//forward jump
            overhead=((offset+sectOffset)/SEFILE_SQL_LOGIC_DATA)*SEFILE_SQL_SECTOR_OVERHEAD;
            dest=absOffset+offset+overhead;
        }
    }
    if(dest<SEFILE_SQL_SECTOR_SIZE){	//pointer inside the header sector is not allowed
        *position=-1;
        return SEFILE_ILLEGAL_SEEK;
    }
    *position=(dest%SEFILE_SQL_SECTOR_SIZE)+(((dest/SEFILE_SQL_SECTOR_SIZE)-1)*SEFILE_SQL_LOGIC_DATA);
    buffer_size=*position-file_length;
    if(buffer_size>0){ 			//if destination exceed the end of the file, empty sectors are inserted at the end of the file to keep the file consistency
        buffer = std::make_unique<uint8_t[]>(buffer_size);
        if(buffer==nullptr){
            return SEFILE_SEEK_ERROR;
        }
        memset(buffer.get(), 0, buffer_size);
        if((file_length % SEFILE_SQL_LOGIC_DATA)){
#if defined(__linux__) || defined(__APPLE__)
        	hTmp->log_offset = lseek(hTmp->fd, ((((file_length/SEFILE_SQL_LOGIC_DATA)+1/* +1 for header */) * SEFILE_SQL_SECTOR_SIZE) + file_length%SEFILE_SQL_LOGIC_DATA), SEEK_SET);
#elif _WIN32
            hTmp->log_offset=SetFilePointer(hTmp->fd, ((((file_length/SEFILE_SQL_LOGIC_DATA)+1/* +1 for header */) * SEFILE_SQL_SECTOR_SIZE) + file_length%SEFILE_SQL_LOGIC_DATA), nullptr, FILE_BEGIN);
#endif
        }
        if(this->securedb_secure_write(buffer.get(), buffer_size)){
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

uint16_t SEfile::securedb_secure_truncate(uint32_t size){
    if((this->l1 == nullptr) || (this->IsOpen == false)){ return SEFILE_TRUNCATE_ERROR; }
	std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
    uint32_t fPosition = 0;
    int rOffset = 0, nSector = 0; //New Relative offset & new number of sectors
    std::unique_ptr<uint8_t[]> buffer;
    uint32_t aSize = 0, bytesRead = 0;
    //    if (secure_sync(hFile)){
    //        return SEFILE_WRITE_ERROR;
    //    }
    if(this->securedb_get_filesize(&aSize)){
        return SEFILE_TRUNCATE_ERROR;
    }
    if(aSize < size){ //File should be enlarged
        if (this->securedb_secure_seek((size-aSize), (int32_t*)&fPosition, SEFILE_END) || fPosition != size+aSize){
            return SEFILE_TRUNCATE_ERROR;
        }
    }else{
        rOffset = size % SEFILE_SQL_LOGIC_DATA; //Relative offset inside a sector
        nSector = (size / SEFILE_SQL_LOGIC_DATA) + 1; //Number of sectors in a file (including header)
#if defined(__linux__) || defined(__APPLE__)
        hTmp->log_offset = lseek(hTmp->fd, nSector*SEFILE_SQL_SECTOR_SIZE, SEEK_SET);
        if(hTmp->log_offset < 0){
            return SEFILE_TRUNCATE_ERROR;
        }
#elif _WIN32
        hTmp->log_offset = SetFilePointer(hTmp->fd, nSector*SEFILE_SQL_SECTOR_SIZE, nullptr, FILE_BEGIN);
        if(hTmp->log_offset == INVALID_SET_FILE_POINTER){
            return SEFILE_TRUNCATE_ERROR;
        }
#endif
        buffer = std::make_unique<uint8_t[]>(rOffset*(sizeof(uint8_t)));
        if(buffer == nullptr){
            return SEFILE_TRUNCATE_ERROR;
        }
        if(this->securedb_secure_read(buffer.get(), rOffset, &bytesRead)){
            return SEFILE_TRUNCATE_ERROR;
        }
#if defined(__linux__) || defined(__APPLE__)
        hTmp->log_offset = lseek(hTmp->fd, nSector*SEFILE_SQL_SECTOR_SIZE, SEEK_SET);
        if(hTmp->log_offset < 0){
            return SEFILE_TRUNCATE_ERROR;
        }
        if(ftruncate(hTmp->fd, nSector*SEFILE_SQL_SECTOR_SIZE)){	//truncate
            return SEFILE_TRUNCATE_ERROR;
        }
#elif _WIN32
        hTmp->log_offset = SetFilePointer(hTmp->fd, nSector*SEFILE_SQL_SECTOR_SIZE, nullptr, FILE_BEGIN);
        if(hTmp->log_offset == INVALID_SET_FILE_POINTER){
            return SEFILE_TRUNCATE_ERROR;
        }
        if(!SetEndOfFile(hTmp->fd)){	//truncate
            return SEFILE_TRUNCATE_ERROR;
        }
#endif
        if(this->securedb_secure_write(buffer.get(), rOffset)){
            return SEFILE_TRUNCATE_ERROR;
        }
    }
    return 0;
}

uint16_t securedb_secure_getfilesize(char *path, uint32_t * position){
    if((path == nullptr) || (position == nullptr)){
    	return SEFILE_FILESIZE_ERROR;
    }
	uint16_t ret = L1Error::Error::OK;
    SEfile currfile;
    currfile.l1 = SEcube;
    if(currfile.securedb_secure_open(path, SEFILE_READ, SEFILE_OPEN) != 0){
        return SEFILE_FILESIZE_ERROR;
    }
    ret = currfile.securedb_get_filesize(position);
    if(currfile.securedb_secure_close()){
        return SEFILE_FILESIZE_ERROR;
    }
    return ret;
}

uint16_t SEfile::securedb_secure_close(){
    if(this->handleptr == nullptr){ return 0; }
#if defined(__linux__) || defined(__APPLE__)
	if(close(this->handleptr->fd) == -1 ){
		//this->handleptr.reset();
		return SEFILE_CLOSE_HANDLE_ERR;
	}
#elif _WIN32
	if ( CloseHandle(this->handleptr->fd) == 0){
		//this->handleptr.reset();
		return SEFILE_CLOSE_HANDLE_ERR;
	}
#endif
	this->handleptr.reset();
	this->IsOpen = false;
    return 0;
}

uint16_t SEfile::securedb_secure_sync(){
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

uint16_t SEfile::securedb_get_filesize(uint32_t *length){
    if((this->l1 == nullptr) || (this->IsOpen == false) || (length == nullptr)){
        return SEFILE_FILESIZE_ERROR;
    }
    uint16_t rc = this->secure_key_check(CryptoInitialisation::Direction::DECRYPT); // check if the key is valid for decryption
    if(rc){ return rc; } // return if the key is not valid
    std::unique_ptr<SEFILE_SQL_SECTOR> crypt_buffer = std::make_unique<SEFILE_SQL_SECTOR>();
    std::unique_ptr<SEFILE_SQL_SECTOR> decrypt_buffer = std::make_unique<SEFILE_SQL_SECTOR>();
    int32_t total_size=0;
    std::shared_ptr<SEFILE_HANDLE> hTmp = this->handleptr;
    if(crypt_buffer==nullptr || decrypt_buffer==nullptr){
        return SEFILE_FILESIZE_ERROR;
    }
#if defined(__linux__) || defined(__APPLE__)
    off_t orig_off;
    size_t BytesRead = 0;
    orig_off=lseek(hTmp->fd, 0, SEEK_CUR);
    total_size=lseek(hTmp->fd, (-1)*(SEFILE_SQL_SECTOR_SIZE), SEEK_END);
    if(orig_off==-1 || total_size==-1){
        return SEFILE_SEEK_ERROR;
    }
    if(!total_size) {
        lseek(hTmp->fd, orig_off, SEEK_SET);
        *length=0;
        return 0;
    }
    if((BytesRead = read(hTmp->fd, crypt_buffer.get(), SEFILE_SQL_SECTOR_SIZE))!= SEFILE_SQL_SECTOR_SIZE){
        lseek(hTmp->fd, orig_off, SEEK_SET);
        return SEFILE_READ_ERROR;
    }
    if((lseek(hTmp->fd, orig_off, SEEK_SET))==-1){
        return SEFILE_SEEK_ERROR;
    }
#elif _WIN32
    DWORD orig_off;
    DWORD BytesRead = 0;
    orig_off=SetFilePointer(hTmp->fd, 0, 0, FILE_CURRENT);
    total_size=SetFilePointer(hTmp->fd, (-1)*(SEFILE_SQL_SECTOR_SIZE), nullptr, FILE_END);
    if(((uint32_t)orig_off)==INVALID_SET_FILE_POINTER || ((uint32_t)total_size)==INVALID_SET_FILE_POINTER){
        return SEFILE_SEEK_ERROR;
    }
    if(!total_size) {
        SetFilePointer(hTmp->fd, orig_off, nullptr, FILE_BEGIN);
        *length=0;
        return 0;
    }
    if ((ReadFile(hTmp->fd, crypt_buffer.get(), SEFILE_SQL_SECTOR_SIZE, &BytesRead, nullptr))==0 || BytesRead!=SEFILE_SQL_SECTOR_SIZE){
        SetFilePointer(hTmp->fd, orig_off, nullptr, FILE_BEGIN);
        return SEFILE_READ_ERROR;
    }
    if((SetFilePointer(hTmp->fd, orig_off, nullptr, FILE_BEGIN))==INVALID_SET_FILE_POINTER){
        return SEFILE_SEEK_ERROR;
    }
#endif
    if (this->decrypt_sectors(crypt_buffer.get(), decrypt_buffer.get(), SEFILE_SQL_SECTOR_DATA_SIZE, securedb_pos_to_cipher_block(total_size), hTmp->nonce_ctr, hTmp->nonce_pbkdf2)){
        return SEFILE_FILESIZE_ERROR;
    }
    if (memcmp(crypt_buffer->signature, decrypt_buffer->signature, B5_SHA256_DIGEST_SIZE)){
        return SEFILE_SIGNATURE_MISMATCH;
    }
    *length=((total_size/SEFILE_SQL_SECTOR_SIZE)-1)*SEFILE_SQL_LOGIC_DATA + decrypt_buffer->len;
    return 0;
}

size_t securedb_pos_to_cipher_block(size_t current_position){
	return ((current_position / SEFILE_SQL_SECTOR_SIZE) - 1) * (SEFILE_SQL_SECTOR_DATA_SIZE / SEFILE_BLOCK_SIZE);
}

uint16_t securedb_ls(std::string& path, std::vector<std::pair<std::string, std::string>>& list, L1* SEcubeptr){
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
    hDir=opendir(path.c_str());
    while((dDir=readdir(hDir)) != nullptr){
        memset(bufferEnc, 0, MAX_PATHNAME*sizeof(char));
        memset(bufferDec, 0, MAX_PATHNAME*sizeof(char));
    	std::string currname(dDir->d_name); // current file or directory name
    	std::string fullpath = s1 + currname;
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
            }else {
            	std::pair<std::string, std::string> p(currname, currname);
            	list.push_back(p);
                return SEFILE_LS_ERROR;
            }
        }else if(dDir->d_type==DT_REG){
            if(valid_file_name(currname) || securedb_decrypt_filename(fullpath, bufferDec, SEcubeptr)){
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
        }else if(dDir.dwFileAttributes==FILE_ATTRIBUTE_DIRECTORY){ // if this is a directory
            if(valid_directory_name(currname)){
            	std::pair<std::string, std::string> p(currname, currname); // name not recognized as generated by SEfile, copy it as it is
            	list.push_back(p);
            	continue;
            }
            if(decrypt_dirname(currname, bufferDec, SEcubeptr) == 0){
            	std::string tmp(bufferDec); // success
            	std::pair<std::string, std::string> p(currname, tmp);
            	list.push_back(p);
            } else {
            	std::pair<std::string, std::string> p(currname, currname);
            	list.push_back(p);
                return SEFILE_LS_ERROR;
            }
        } else { // if this is a file
            if(valid_file_name(currname) || securedb_decrypt_filename(fullpath, bufferDec, SEcubeptr)){
            	std::pair<std::string, std::string> p(currname, currname); // this may fail in case of invalid name or encryption key that was destroyed
            	list.push_back(p);
                continue;
            }
            if(crypto_filename(bufferDec, bufferEnc, &encoded_length)){
            	std::pair<std::string, std::string> p(currname, currname); // this may fail in case of invalid name or encryption key that was destroyed
            	list.push_back(p);
                return SEFILE_LS_ERROR;
            }
            if(currname.compare(0, std::string::npos, bufferEnc, encoded_length) == 0){
            	std::string tmp(bufferDec);
            	std::pair<std::string, std::string> p(currname, tmp); // this may fail in case of invalid name or encryption key that was destroyed
            	list.push_back(p);
            } else {
            	std::pair<std::string, std::string> p(currname, currname); // error
            	list.push_back(p);
                return SEFILE_LS_ERROR;
            }
        }
    }while(FindNextFile(hDir, &dDir));
    FindClose(hDir);
#endif
    return 0;
}

uint16_t securedb_decrypt_filename(std::string& path, char *filename, L1 *SEcubeptr){
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
	std::unique_ptr<SEFILE_SQL_SECTOR> header_buffer = std::make_unique<SEFILE_SQL_SECTOR>();
	std::unique_ptr<SEFILE_SQL_SECTOR> bufferDec = std::make_unique<SEFILE_SQL_SECTOR>();
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
    if(read(currfile.handleptr->fd, header_buffer.get(), SEFILE_SQL_SECTOR_SIZE)!=SEFILE_SQL_SECTOR_SIZE){
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
    if(ReadFile(currfile.handleptr->fd, header_buffer.get(), SEFILE_SQL_SECTOR_SIZE, &BytesRead, nullptr)==0 || BytesRead!=SEFILE_SQL_SECTOR_SIZE){ // read the first sector
        return SEFILE_FILENAME_DEC_ERROR;
    }
    if(SetFilePointer(currfile.handleptr->fd, orig_off, nullptr, FILE_BEGIN)==INVALID_SET_FILE_POINTER){ // restore file pointer
        return SEFILE_FILENAME_DEC_ERROR;
    }
#endif
    currfile.EnvKeyID = header_buffer->header.key_header.key_id; // retrieve ID of key used to encrypt this file
    currfile.EnvCrypto = header_buffer->header.key_header.algorithm; // retrieve ID of algorithm used to encrypt this file
    uint8_t length=0;
    if(currfile.crypt_header(header_buffer.get(), bufferDec.get(), SEFILE_SQL_SECTOR_DATA_SIZE, CryptoInitialisation::Direction::DECRYPT)){ // decrypt the header
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

uint16_t securedb_recrypt(std::string& path, uint32_t key, L1 *SEcubeptr){
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
		SEfile newfile(SEcubeptr, key);
		newfile.EnvCrypto = L1Algorithms::Algorithms::AES_HMACSHA256;
		override_key_check = true;
		if(securedb_secure_getfilesize((char*)path.c_str(), &oldsize) ||
		   oldfile.securedb_secure_open((char*)path.c_str(), SEFILE_READ, SEFILE_OPEN) ||
		   newfile.securedb_secure_open((char*)newfilename.c_str(), SEFILE_WRITE, SEFILE_NEWFILE) ||
		   oldfile.securedb_secure_seek(0, &pos, SEFILE_BEGIN)){
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
			if(oldfile.securedb_secure_read(buffer, toread, &bytesread) == 0){
				if(bytesread > 0){
					if(newfile.securedb_secure_write(buffer, bytesread)){
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
		remove(enc_filename_old); // delete old file
		rename(enc_filename_new, enc_filename_old); // rename re-encrypted file
		override_key_check = false;
		return 0;
	} catch (...) {
		remove(enc_filename_new); // error, remove new file
		override_key_check = false;
		throw;
	}
}

uint16_t securedb_get_secure_context(std::string& filename, std::string *keyid, uint16_t *algo){
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
		uint16_t rc = encryptedfile.securedb_secure_open(p.get(), SEFILE_READ, SEFILE_OPEN);
		override_key_check = false;
		if(rc){	return -1; }
	} catch (...) {
		override_key_check = false;
		return -1;
	}
	keyid->assign("K" + std::to_string(encryptedfile.EnvKeyID));
	*algo = encryptedfile.EnvCrypto;
	if(encryptedfile.securedb_secure_close()){
		return -1;
	}
	return 0;
}

/* SEcure Database APIs exposed to SQLite. Do not use these functions outside of sqlite3.c */
extern uint16_t c_sql_secure_open(char *path, SEFILE_FHANDLE *hFile, int32_t mode, int32_t creation){
	try{
		if((hFile == nullptr) || (path == nullptr)){ return SEFILE_OPEN_ERROR; }
		char filename[MAX_PATHNAME];
		memset(filename, '\0', MAX_PATHNAME);
		get_filename(path, filename);
		std::string fname(filename);
		if(fname.empty()){ return SEFILE_OPEN_ERROR; }
		for(std::vector<std::unique_ptr<SEfile>>::iterator it = databases.begin(); it != databases.end(); it++){ /*  search for a name relatable to this path in the database list */
				if((*it)->handleptr == nullptr){ continue; }
				std::string dbname((*it)->handleptr->name);
				if(dbname.compare(fname) == 0){ // perfect match with SEfile object inserted in list
					uint16_t rc = (*it)->securedb_secure_open(path, mode, creation); // use the current SEfile object
					memset((*it)->handleptr->name, '\0', MAX_PATHNAME);
					for(unsigned int i=0; i<fname.length(); i++){
						(*it)->handleptr->name[i] = fname.at(i); // copy the name of the file to the SEfile object
					}
					*hFile = (*it)->handleptr.get(); // copy the pointer of the SEFILE_HANDLE instance to the pointer used by SQLite
					return rc;
				} else {
					if((dbname.find(fname) != std::string::npos) || ((fname.find(dbname) != std::string::npos) && (dbname.empty() == false))){ // still a match, but not perfect
						std::unique_ptr<SEfile> duplicate = std::make_unique<SEfile>(); // the file being opened is somehow related to an object already in the list (i.e. it may be a temp file) so duplicate the object to exploit the same security context on another file
						duplicate->EnvCrypto = (*it)->EnvCrypto;
						duplicate->EnvKeyID = (*it)->EnvKeyID;
						duplicate->l1 = (*it)->l1;
						uint16_t rc = duplicate->securedb_secure_open(path, mode, creation); // use the duplicated SEfile object
						memset(duplicate->handleptr->name, '\0', MAX_PATHNAME);
						for(unsigned int i=0; i<fname.length(); i++){
							duplicate->handleptr->name[i] = fname.at(i); // copy the name of the file to the duplicated SEfile object
						}
						*hFile = duplicate->handleptr.get(); // copy the pointer of the SEFILE_HANDLE instance to the pointer used by SQLite
						databases.push_back(std::move(duplicate));
						return rc;
					}
				}
		}
		// at this point, the name does not match anything we know, so we return an error
		return SEFILE_OPEN_ERROR;
	} catch(...){
		return SEFILE_OPEN_ERROR;
	}
}
extern uint16_t c_sql_secure_write(SEFILE_FHANDLE *hFile, uint8_t * dataIn, uint32_t dataIn_len){
	try{
		if((hFile == nullptr) || (dataIn == nullptr) || (*hFile == nullptr)){
			return SEFILE_WRITE_ERROR;
		}
		uint16_t rc = 0;
		for(std::vector<std::unique_ptr<SEfile>>::iterator it = databases.begin(); it != databases.end(); it++){ // search for the fd in the database table
			if((*it)->handleptr == nullptr){ continue; }
			if((*it)->handleptr.get() == *hFile){
				rc = (*it)->securedb_secure_write(dataIn, dataIn_len);
				return rc;
			}
		}
		return SEFILE_WRITE_ERROR;
	} catch(...){
		return SEFILE_WRITE_ERROR;
	}
}
extern uint16_t c_sql_secure_read(SEFILE_FHANDLE *hFile,  uint8_t * dataOut, uint32_t dataOut_len, uint32_t * bytesRead){
	try{
		if((hFile==nullptr) || (*hFile==nullptr) || (dataOut==nullptr)){ return SEFILE_READ_ERROR; }
		uint16_t rc = 0;
		for(std::vector<std::unique_ptr<SEfile>>::iterator it = databases.begin(); it != databases.end(); it++){ // search for the fd in the database table
			if((*it)->handleptr == nullptr){ continue; }
			if((*it)->handleptr.get() == *hFile){
				rc = (*it)->securedb_secure_read(dataOut, dataOut_len, bytesRead);
				return rc;
			}
		}
		return SEFILE_READ_ERROR;
	} catch(...){
		return SEFILE_READ_ERROR;
	}
}
extern uint16_t c_sql_secure_seek(SEFILE_FHANDLE *hFile, int32_t offset, int32_t *position, uint8_t whence){
	try{
		if((hFile==nullptr) || (*hFile==nullptr) || (position==nullptr)){ return 1; }
		uint16_t rc = 0;
		for(std::vector<std::unique_ptr<SEfile>>::iterator it = databases.begin(); it != databases.end(); it++){ // search for the fd in the database table
			if((*it)->handleptr == nullptr){ continue; }
			if((*it)->handleptr.get() == *hFile){
				rc = (*it)->securedb_secure_seek(offset, position, whence);
				return rc;
			}
		}
		return 1;
	} catch(...){
		return 1;
	}
}
extern uint16_t c_sql_secure_truncate(SEFILE_FHANDLE *hFile, uint32_t size){
	try{
		if((hFile==nullptr) || (*hFile==nullptr)){ return SEFILE_TRUNCATE_ERROR; }
		uint16_t rc = 0;
		for(std::vector<std::unique_ptr<SEfile>>::iterator it = databases.begin(); it != databases.end(); it++){ // search for the fd in the database table
			if((*it)->handleptr == nullptr){ continue; }
			if((*it)->handleptr.get() == *hFile){
				rc = (*it)->securedb_secure_truncate(size);
				return rc;
			}
		}
		return SEFILE_TRUNCATE_ERROR;
	} catch(...){
		return SEFILE_TRUNCATE_ERROR;
	}
}
extern uint16_t c_sql_secure_getfilesize(char *path, uint32_t * position){
	return securedb_secure_getfilesize(path, position);
}
extern uint16_t c_secure_sync(SEFILE_FHANDLE *hFile){
	try{
		if((hFile==nullptr) || (*hFile==nullptr)){ return SEFILE_SYNC_ERR; }
		uint16_t rc = 0;
		for(std::vector<std::unique_ptr<SEfile>>::iterator it = databases.begin(); it != databases.end(); it++){ // search for the fd in the database table
			if((*it)->handleptr == nullptr){ continue; }
			if((*it)->handleptr.get() == *hFile){
				rc = (*it)->securedb_secure_sync();
				return rc;
			}
		}
		return SEFILE_SYNC_ERR;
	} catch(...){
		return SEFILE_SYNC_ERR;
	}
}
extern uint16_t c_secure_close(SEFILE_FHANDLE *hFile){
	try{
		if((hFile==nullptr) || (*hFile==nullptr)){ return SEFILE_CLOSE_HANDLE_ERR; }
		uint16_t rc = 0;
		for(std::vector<std::unique_ptr<SEfile>>::iterator it = databases.begin(); it != databases.end(); it++){ // search for the fd in the database table
			if((*it)->handleptr == nullptr){ continue; }
			if((*it)->handleptr.get() == *hFile){
				rc = (*it)->securedb_secure_close();
				*hFile = nullptr;
				databases.erase(it); // remove from the database
				return rc;
			}
		}
		return SEFILE_CLOSE_HANDLE_ERR;
	} catch(...){
		return SEFILE_CLOSE_HANDLE_ERR;
	}
}
