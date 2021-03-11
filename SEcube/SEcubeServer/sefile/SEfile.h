/**
  ******************************************************************************
  * File Name          : SEfile.h
  * Description        : SEfile library header.
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

/*! \file  SEfile.h
 *  \brief This is the SEfile header that contains most of the methods, constants and structures used by SEfile.
 *  \author Fornero Matteo
 *  \date 25/11/2019
 */

#ifndef SEFILE_H_
#define SEFILE_H_

#include "../sources/L1/L1.h"
#include "SEfile_C_interface.h"

#define KEY_CHECK_INTERVAL 1 /**<  @brief Time interval (in seconds) used to check for the validity of the key used to encrypt the file. */
#define SEFILE_NONCE_LEN 32
extern bool override_key_check; /**<  @brief Global flag that is used to bypass the validity check of a key to read or write encrypted data. It is used only to re-encrypt data belonging to a compromised file. */

/**  @brief Length of header sector reserved to SEkey informations.
 * @details This is the length of the header required by SEkey. it is embedded in the SEfile header;
 * the length of the SEkey header must be a multiple of the block size used by the cipher
 * (16 bytes for AES256 at the moment) because the size of the data to cipher inside the
 * first sector is 512 - SEFILE_NONCE_LEN - SEKEY_HDR_LEN and it must be a multiple of 16. */
#define SEKEY_HDR_LEN 16

/** @brief The SEFILE_HANDLE struct
 * This abstract data type is used to hide from higher level of abstraction its implementation. The data stored in here are the current physical file pointer position and the file descriptor OS-dependent data type. */
#pragma pack(push,1)
struct SEFILE_HANDLE {
    uint32_t log_offset;    /**< Actual pointer position in bytes*/
#if defined(__linux__) || defined(__APPLE__)
    int32_t fd;             /**< File descriptor in Unix environment*/
#elif _WIN32
    HANDLE fd;              /**< File descriptor in Windows environment*/
#endif
    uint8_t nonce_ctr[16];  /**< Nonce used for the CTR feedback*/
    uint8_t nonce_pbkdf2[SEFILE_NONCE_LEN]; /**< Nonce used for the PBKDF2*/
    char name[MAX_PATHNAME]; /*< String that contains the name of the file. This is exploited only by the SEcure Database in order to run other databases apart from the one of SEkey. */
    SEFILE_HANDLE();
};

/** @brief The header of each file encrypted with SEfile.
 * This is the header that contains info for the SEkey system. It is stored as cleartext and it is not authenticated.
 * The dimension is always \ref SEKEY_HDR_LEN. */
struct SEKEY_HEADER {
	uint32_t key_id;		/**< The ID of the key used to encrypt this file. */
	uint16_t algorithm;		/**< The algorithm used to encrypt this file. */
	uint8_t padding[10];	/**< Padding used to reach a SEkey header size which is a multiple of 16. */
};

/** @brief The SEFILE_HEADER struct
 * This data struct is used to define a 31 bytes field inside a sector
 * while taking care of its inner composition. */
struct SEFILE_HEADER {
	uint8_t nonce_pbkdf2[SEFILE_NONCE_LEN];	/**< 32 random bytes storing the IV for generating a different key*/
	SEKEY_HEADER key_header; /**< The header with the ID of the key and the algorithm. */
    uint8_t nonce_ctr[16];		            /**< 16 random bytes storing the IV for next sectors*/
    int32_t magic;				            /**< 4 bytes used to represent file type (not used yet)*/
    int16_t ver;				            /**< 2 bytes used to represent current filesystem version (not used yet)*/
    int32_t uid;				            /**< 4 bytes not used yet*/
    int32_t uid_cnt;			            /**< 4 bytes not used yet*/
    uint8_t fname_len;			            /**< 1 byte to express how long is the filename.*/
};

/** @brief The SEFILE_SECTOR struct
 * This data struct is the actual sector organization.
 * The total size should ALWAYS be equal to \ref SEFILE_SECTOR_SIZE.
 * The first sector is used to hold ONLY the header.
 * Thanks to the union data type, the developer can simply declare
 * a sector and then choose if it is the header sector or not. */
struct SEFILE_SECTOR {
	// notice that the size of the SEFILE_SECTOR does depend on the union. The size of the union is the size of the largest data inside the union (plus memory alignment if any).
	union {
        SEFILE_HEADER header;               /**< See \ref SEFILE_HEADER .*/
        uint8_t data[SEFILE_LOGIC_DATA];   /**< In here it will be written the actual data.
         *   Since it is inside a union data type, the
         *   filename will be written from 32nd byte.
         */
    };
    uint16_t len;                      /**< How many bytes are actually stored in this sector.*/
    uint8_t signature[32];             /**< Authenticated digest generated by the device*/
    SEFILE_SECTOR(); /**< Constructor used to initialize all the fields of the struct to zero. */
};
#pragma pack(pop)

/* functions not related to SEfile objects that can be called by higher levels */
/** @brief This function retrieves the key ID and the algorithm used to encrypt the file specified by filename.
* @param [in] filename Absolute or relative path of the file.
* @param [out] keyid The ID of the key used to encrypt the file.
* @param [out] algo The algorithm used to encrypt the file.
* @return The function returns 0 in case of success. See \ref errorValues for error list. */
uint16_t get_secure_context(std::string& filename, std::string *keyid, uint16_t *algo);
/** @brief This function creates a directory with an encrypted name.
* @param [in] path Absolute or relative path of the new directory.
* @param [in] SEcubeptr Pointer to the L1 object used to communicate with the SEcube.
* @param [in] key ID of the key to be used to encrypt the name of the new directory.
* @return The function returns 0 in case of success. See \ref errorValues for error list. */
uint16_t secure_mkdir(std::string& path, L1 *SEcubeptr, uint32_t key);
/** @brief This function identifies which encrypted files and encrypted directories are present in the directory pointed by path and writes them in list.
* @param [in] path Absolute or relative path to the directory to browse.
* @param [out] list List of pairs containing the encrypted name and the decrypted name.
* @param [in] SEcubeptr Pointer to the L1 object used to communicate with the SEcube.
* @return The function returns a 0 in case of success. See \ref errorValues for error list.
* @details Notice that, if the name of a file or of a directory belonging to the path is not associated to SEfile, then it is copied as it is in the list. This function is not recursive. */
uint16_t secure_ls(std::string& path, std::vector<std::pair<std::string, std::string>>& list, L1 *SEcubeptr);
/** @brief This function is used to get the total logic size of an encrypted file pointed by path. Logic size will always be smaller than physical size because it takes into account the overhead introduced by SEfile.
* @param [in] path Absolute or relative path the file.
* @param [out] position Where the size of the file is stored.
* @param [in] SEcubeptr Pointer to the L1 object used to communicate with the SEcube.
* @return The function returns 0 in case of success. See \ref errorValues for error list. */
uint16_t secure_getfilesize(char *path, uint32_t * position, L1 *SEcubeptr);
/** @brief This function re-encrypts an encrypted file pointed by path with the new key specified as parameter.
* @param [in] path Absolute or relative path of the file.
* @param [in] key The ID of the key used to encrypt the file.
* @param [in] SEcubeptr Pointer to the L1 object used to communicate with the SEcube.
* @return The function returns 0 in case of success. See \ref errorValues for error list.
* @details This function should be used to re-encrypt a file that was encrypted with a key that is not trusted anymore (i.e. a compromised key). */
uint16_t secure_recrypt(std::string path, uint32_t key, L1 *SEcubeptr);

/* functions that are not related to SEfile objects that should not be called by higher levels because they are used internally by SEfile methods */
/** @brief This function is used to compute the plaintext of an encrypted filename stored in path.
* @param [in] path Where the encrypted file is stored, it can be an absolute or relative path. No encrypted directory names are allowed.
* @param [out] filename A preallocated string where to store the plaintext filename.
* @param [in] SEcubeptr Pointer to the L1 object used to communicate with the SEcube.
* @return The function returns 0 in case of success. See \ref errorValues for error list. */
uint16_t decrypt_filename(std::string& path, char *filename, L1 *SEcubeptr);
/** @brief This function is used to compute the ciphertext of a directory name stored in dirname.
* @param [in] path Path to the directory whose name has to be encrypted.
* @param [out] encDirname A preallocated string where to store the encrypted directory name.
* @param [out] enc_len Where to store how many bytes the encrypted directory name is long. Can be NULL.
* @param [in] SEcubeptr Pointer to the L1 object used to communicate with the SEcube.
* @param [in] key ID of the key to be used to encrypt the name of the new directory.
* @return The function returns 0 in case of success. See \ref errorValues for error list. */
uint16_t crypt_dirname(std::string& path, char *encDirname, uint32_t* enc_len, L1 *SEcubeptr, uint32_t key);
/** @brief This function is used to compute the plaintext of am encrypted directory name stored in dirname.
* @param [in] dirpath Path to the directory whose name has to be decrypted.
* @param [out] decDirname A preallocated char array where to store the decrypted directory name.
* @param [in] SEcubeptr Pointer to the L1 object used to communicate with the SEcube.
* @return The function returns 0 in case of success. See \ref errorValues for error list. */
uint16_t decrypt_dirname(std::string& path, char *decDirname, L1 *SEcubeptr);
uint16_t encrypt_name(void* buff1, void* buff2, size_t size, uint16_t direction, L1 *SEcubeptr, uint32_t key); /**< @brief Internally used by crypt_dirname and decrypt_dirname(). */
/** @brief This function checks if the given name can be a valid SEfile directory name.
 * @param [in] name Name of the directory.
 * @return The function returns 0 in case of success. See \ref errorValues for error list.
 * @details This function takes as input the name of a directory and checks if it matches the
 * requirements of an encrypted directory name create with secure_mkdir() (i.e. alphanumeric characters,
 * minimum number of characters, etc.).*/
uint16_t valid_directory_name(std::string& name);
/** @brief This function checks if the given name can be a valid SEfile file name.
 * @param [in] name Name of the file.
 * @return The function returns 0 in case of success. See \ref errorValues for error list.
 * @details This function takes as input the name of a file and checks if it matches the
 * requirements of an encrypted file name created with crypto_filename() (i.e. alphanumeric characters,
 * minimum number of characters, etc.).*/
uint16_t valid_file_name(std::string& name);
size_t pos_to_cipher_block(size_t current_position); /**< @brief Internally used by SEfile functions. */
void compute_blk_offset(size_t current_offset, uint8_t* nonce); /**< @brief Internally used by SEfile functions. */
void get_filename(char *path, char *file_name); /**< @brief Extract the name of a file from its path. */
void get_path(char *full_path, char *path); /**< @brief Extract the path of a file removing the file name. */

/**
* \class SEfile
* @brief A SEfile object is used to manage a file encrypted with SEfile.
* @details Each SEfile object has several attributes that define the security context. A security context is made of a key, an algorithm and a SEcube.
* These three parameters define how the file must be protected (i.e. AES-256 encrypted using the key with ID 10). There are also other attributes
* that are used for internal purposes. The class internally uses smart pointers and the RAII approach to guarantee correct memory management. Notice that
* the attributes are public just for ease of use.
*/
class SEfile{
public:
	 uint32_t EnvKeyID; /**< @brief The key ID used by this SEfile instance. This key will be used for encryption and decryption. */
	 uint16_t EnvCrypto; /**<  @brief The algorithm to be used with the key. */
	 time_t LastEncryptCheckTime; /**<  @brief The last time the validity of the key used by this file was checked, requiring write (encrypt, requires active key) privilege. */
	 time_t LastDecryptCheckTime; /**<  @brief The last time the validity of the key used by this file was checked, requiring read (decrypt, does not require active key) privilege. */
	 bool IsOpen; /**<  @brief Flag that is TRUE if the file is open, FALSE otherwise. */
	 L1 *l1; /**<  @brief The pointer to the L1 object created by the main application (i.e. to login to the SEcube). */
	 std::shared_ptr<SEFILE_HANDLE> handleptr; /**<  @brief Handle to the file on the underlying filesystem. */
	 /* Notice that a shared_ptr is used for the SEFILE_HANDLE structure because it is more manageable by other components of the SEcube SDK (i.e. SEkey KMS and the SEcure Database).
	  * Considering SEfile only, having a pointer or having directly the structure inside the class makes no difference...but it makes a difference when using SEfile together with SQLite
	  * for the SEcure Database. Therefore, in order to keep the same object for the normal SEfile version and for the SEfile of the SEcure DB, the smart pointer is better. */
	 SEfile(); /**<  @brief Default constructor. Initializes the secure environment with empty values. */
	 SEfile(L1 *secube); /**<  @brief Constructor to initialize the secure environment with empty values, apart from the pointer to the SEcube to be used. */
	 SEfile(L1 *secube, uint32_t keyID); /**<  @brief Constructor to initialize the secure environment with empty values, apart from the pointer to the SEcube to be used and the ID of the key to be used. */
	 SEfile(L1 *secube, uint32_t keyID, uint16_t crypto); /**<  @brief Constructor to fully initialize the secure environment. */
	 ~SEfile(); /**<  @brief Destructor. Automatically calls secure_finit() and secure_close(). */
	 /* APIs of SEfile */
	 /** @brief This function is used to initialize the security context of a SEfile object.
	 * @return The function returns a (uint16_t) '0' in case of success. See \ref errorValues for error list.
	 * @details Notice that this function works similarly to the SEfile constructor with the same parameters.
	 * 		   If you specify a key that is not stored in the SEcube or an algorithm that is not supported by
	 * 		   SEfile, this function will return an error. */
	 uint16_t secure_init(L1 *l1ptr, uint32_t keyID, uint16_t crypto);
	 /** @brief This function resets the parameters set by the secure_init() to default values (0s and NULL). */
	 void secure_finit();
	 /**
	  * @defgroup sefile_apis
	  *  @{
	  */
	 	/**
	 	 * \name APIs of SEfile
	 	 * \details These APIs are the most useful to exploit the SEfile library.
	 	 */
	 	///@{

			 /** @brief This function opens or creates a secure file managed with SEfile.
			 * @param [in] path The absolute/relative path (plaintext, i.e. myfile.txt) where to open or create the file.
			 * @param [in] mode The mode in which the file should be opened. See \ref Mode_Defines.
			 * @param [in] creation Define if the file should be created or it should already exist. See \ref Creation_Defines.
			 * @return The function returns 0 in case of success. See \ref errorValues for error list.
			 * @details Notice that you must specify if you want to open the file or if you want to create it. There is not the
			 * possibility of "create if the file does not exist", therefore if you need such behaviour you must check in advance
			 * if the file exists or not (i.e. computing the encrypted name of the file with crypto_filename() and then checking
			 * with OS system calls if the file exists). */
	 	 	 uint16_t secure_open(char *path, int32_t mode, int32_t creation);

			/** @brief This function releases the resources related to the underlying SEfile object (i.e. closes the file descriptor).
			* @return The function returns 0 in case of success. See \ref errorValues for error list. */
			 uint16_t secure_close();

			 /** @brief This function reads dataOut_len bytes into dataOut from the file descriptor managed by the underlying SEfile object.
			 * @param [out] dataOut An already allocated array of characters where to store data read.
			 * @param [in] dataOut_len Number of characters we want to read.
			 * @param [out] bytesRead Number of effective characters read, it cannot be NULL.
			 * @return The function returns 0 in case of success. See \ref errorValues for error list. */
			 uint16_t secure_read(uint8_t *dataOut, uint32_t dataOut_len, uint32_t *bytesRead);

			 /** @brief This function writes the bytes stored at dataIn to the encrypted file managed by the SEfile object on which this method is called.
			 * @param [in] dataIn The array of bytes that have to be written.
			 * @param [in] dataIn_len The length, in bytes, of the data that have to be written.
			 * @return The function returns a 0 in case of success. See \ref errorValues for error list. */
			 uint16_t secure_write(uint8_t *dataIn, uint32_t dataIn_len);

			 /** @brief This function is used to move the file pointer of a file managed by a SEfile object.
			 * @param [in] offset Amount of bytes we want to move.
			 * @param [out] position Pointer to a int32_t variable where the final position is stored, it cannot be NULL.
			 * @param [in] whence According to this parameter we can choose if we want to move from the file beginning, file ending or current file pointer position. See \ref Seek_Defines.
			 * @return The function returns 0 in case of success. See \ref errorValues for error list. */
			 uint16_t secure_seek(int32_t offset, int32_t *position, uint8_t whence);

			 /** @brief This function resizes the file managed by the underlying SEfile object to size bytes. If size is bigger than its current size the gap is filled with 0s.
			 * @param [in] size  New size of the file.
			 * @return The function returns 0 in case of success. See \ref errorValues for error list. */
			 uint16_t secure_truncate(uint32_t size);

			 /** @brief This function is used in case we want to be sure that the physical file is synced with the OS buffers.
			 * @return The function returns 0 in case of success. See \ref errorValues for error list. */
			 uint16_t secure_sync();
		///@}
	/** @}*/
	 /**
	  * @defgroup sefile_internal_functions
	  *  @{
	  */
	 	/**
	 	 * \name Functions for internal purposes of SEfile
	 	 * \details You should not need to use these functions directly because they are used internally by the SEfile library.
	 	 */
	 	///@{

			 /** @brief This function creates a new secure file managed with SEfile. If the file already exists, it is overwritten
			 *        with an empty one, all previous data are lost.
			 * @param [in] path Specify the absolute/relative path where to create the file. No encrypted directory are allowed inside the path.
			 * @param [out] hFile The pointer in which the file handle to the new opened file is placed after a success, NULL in case of failure.
			 * @param [in] mode The mode in which the file should be created. See \ref Mode_Defines.
			 * @return The function returns 0 in case of success. See \ref errorValues for error list.
			 * @details You do not need to call this function explicitly. Use secure_open() instead. */
			 uint16_t secure_create(char *path, std::shared_ptr<SEFILE_HANDLE> hFile, int mode);

			 /** @brief This function is used to check if the key setup in the SEfile object can be used to encrypt or decrypt data.
			 * @param [in] direction Check if the key can be used for encryption or decryption.
			 * @return The function returns 0 in case of success. See \ref errorValues for error list. This function works as SEfile::secure_getfilesize(). */
			 uint16_t secure_key_check(uint16_t direction);

			 /** @brief This function is used to compute the total logic size of a file that is already open within a SEfile object.
			 * @param [out] length Where the logic size of the file is stored.
			 * @return The function returns 0 in case of success. See \ref errorValues for error list. This function works as SEfile::secure_getfilesize(). */
			 uint16_t get_filesize(uint32_t * length);

			 /** \brief This function encrypts a header buffer by exploiting the functions provided by \ref L1.h.
			 * \param [in] buff1 Pointer to the header we want to encrypt/decrypt.
			 * \param [out] buff2 Pointer to an allocated header where to store the result.
			 * \param [in] datain_len How big is the amount of data.
			 * \param [in] direction See \ref SE3_DIR.
			 * \return The function returns 0 in case of success. See \ref errorValues for error list. */
			 uint16_t crypt_header(void *buff1, void *buff2, size_t datain_len, uint16_t direction);

			 /** \brief This function encrypts the buff_decrypt data by exploiting the functions provided by \ref L1.h.
			 *  \param [in] buff_decrypt The plaintext data to be encrypted
			 *  \param [out] buff_crypt  The preallocated buffer where to store the encrypted data.
			 *  \param [in] datain_len Specify how many data we want to encrypt.
			 *  \param [in] current_offset Current position inside the file expressed as number of cipher blocks
			 *  \param [in] nonce_ctr Initialization vector, see \ref SEFILE_HEADER
			 *  \param [in] nonce_pbkdf2 Initialization vector, see \ref SEFILE_HEADER
			 *  \return The function returns 0 in case of success. See \ref errorValues for error list. */
			 uint16_t crypt_sectors(void *buff_decrypt, void *buff_crypt, size_t datain_len, size_t current_offset, uint8_t* nonce_ctr, uint8_t* nonce_pbkdf2);

			 /** \brief This function decrypts the buff_crypt data by exploiting the functions provided by \ref L1.h.
			 *  \param [in] buff_crypt The cipher text data to be decrypted
			 *  \param [out] buff_decrypt  The preallocated buffer where to store the decrypted data.
			 *  \param [in] datain_len Specify how many data we want to decrypt.
			 *  \param [in] current_offset Current position inside the file expressed as number of cipher blocks
			 *  \param [in] nonce_ctr Initialization vector, see \ref SEFILE_HEADER
			 *  \param [in] nonce_pbkdf2 Initialization vector, see \ref SEFILE_HEADER
			 *  \return The function returns 0 in case of success. See \ref errorValues for error list. */
			 uint16_t decrypt_sectors(void *buff_crypt, void *buff_decrypt, size_t datain_len, size_t current_offset, uint8_t* nonce_ctr, uint8_t* nonce_pbkdf2);
		///@}
	/** @}*/
	 /**
	  * @defgroup sefile_custom_sqlite_apis
	  *  @{
	  */
	 	/**
	 	 * \name APIs of SEfile for SQLite DB engine
	 	 * \details These APIs customized to apply SEfile to the SQLite db engine. They are called automatically by the custom VFS of SQLite;
	      * they are required by SEkey and by the SEcure Database library. You should not need to use these APIs directly.
	 	 */
	 	///@{
			 uint16_t securedb_secure_open(char *path, int32_t mode, int32_t creation);
			 uint16_t securedb_secure_close();
			 uint16_t securedb_secure_read(uint8_t * dataOut, uint32_t dataOut_len, uint32_t *bytesRead);
			 uint16_t securedb_secure_write(uint8_t * dataIn, uint32_t dataIn_len);
			 uint16_t securedb_secure_seek(int32_t offset, int32_t *position, uint8_t whence);
			 uint16_t securedb_secure_truncate(uint32_t size);
			 uint16_t securedb_secure_sync();
			 uint16_t securedb_get_filesize(uint32_t * length);
			 uint16_t securedb_secure_create(char *path, std::shared_ptr<SEFILE_HANDLE> hFile, int mode);
		///@}
	/** @}*/
};

#endif
