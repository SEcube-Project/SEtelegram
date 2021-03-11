/**
  ******************************************************************************
  * File Name          : SEfile_C_interface.h
  * Description        : This file includes some of the constants, return values and public functions used for implementing a secure file system.
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

/*! \file  SEfile_C_interface.h
 *  \brief This file includes some of the constants, return values and public functions used for implementing a secure file system.
 *  \authors Matteo Fornero, Francesco Giavatto, Nicolo Maunero, Giulio Scalia
 *  \date 17/09/2016
 *
 * This header file was created moving part of the content of SEfile.h here. This separate header file is required because it must
 * be included by SQLite in order to implement SEkey and to support encrypted SQLite databases based on SEfile. */

#ifndef SEFILE_C_INTERFACE_H_
#define SEFILE_C_INTERFACE_H_

#include <stdint.h>

#ifdef __linux__
	///    @cond linuxDef
	#include <sys/types.h>	/* open, seek */
	#include <sys/stat.h>	/* open, create */
	#include <fcntl.h>		/* open, create */
	#include <unistd.h>		/* write, read, seek, close */
	#include <libgen.h>
	#include <dirent.h>
	///     @endcond
#elif _WIN32
	#undef UNICODE
	#undef _UNICODE
	#ifdef __MINGW32__
	#include <windows.h>
	#else
	#include <Windows.h>
	#endif
#elif __APPLE__
	#include <fcntl.h>		/* open, create */
	#include <sys/stat.h>	/* open */
	#include <unistd.h>		/* write, read, seek, close */
	#include <sys/types.h>	/* open, read */
	#include <sys/uio.h>	/* read */
	#include <dirent.h>
#endif

/**
 * \defgroup Mode_Defines mode parameter for secure_open
 * @{
 */
	/** \name Use this values as mode parameter for secure_open().
	 */
	///@{
		#ifdef __linux__
			#define SEFILE_READ		O_RDONLY    /**< Open as Read only */
			#define SEFILE_WRITE	O_RDWR     /**< Open for Read/Write */
		#elif _WIN32
			#define SEFILE_READ		GENERIC_READ /**< Open as Read only */
			#define SEFILE_WRITE	GENERIC_READ | GENERIC_WRITE /**< Open for Read/Write */
		#elif __APPLE__
			#define SEFILE_READ		O_RDONLY /**< Open as Read only */
			#define SEFILE_WRITE	O_RDWR /**< Open for Read/Write */
		#endif
	///@}
/** @}*/

/**
 * \defgroup Creation_Defines creation parameter for secure_open
 * @{
 */
	/** \name Use this values as creation parameter for secure_open().
	 */
	///@{
		#ifdef __linux__
			#define SEFILE_NEWFILE	O_CREAT | O_TRUNC /**< Create new file and delete if existing */
			#define SEFILE_OPEN		0                 /**< Open an existing file, create it if not existing */
		#elif _WIN32
			#define SEFILE_NEWFILE	CREATE_ALWAYS /**< Forces file creation even if the file already exists. */
			#define SEFILE_OPEN		OPEN_EXISTING /**< Opens only if the file already exists. */
		#elif __APPLE__
			#define SEFILE_NEWFILE	O_CREAT | O_TRUNC /**< Create new file and delete if existing */
			#define SEFILE_OPEN		0                 /**< Open an existing file, create it if not existing */
		#endif
	///@}
/** @}*/

/**
 * \defgroup Seek_Defines whence parameter for secure_seek
 * @{
 */
	/** \name Use this values as whence parameter for secure_seek().
	 */
	///@{
		#ifdef __linux__
			#define SEFILE_BEGIN 	SEEK_SET    /**< Seek from file beginning */
			#define SEFILE_CURRENT 	SEEK_CUR    /**< Seek from current position */
			#define SEFILE_END 		SEEK_END    /**< Seek from file ending */
		#elif _WIN32
			#define SEFILE_BEGIN 	FILE_BEGIN  /**< Seek from file beginning */
			#define SEFILE_CURRENT 	FILE_CURRENT /**< Seek from current position */
			#define SEFILE_END 		FILE_END /**< Seek from file ending */
		#elif __APPLE__
			#define SEFILE_BEGIN 	SEEK_SET    /**< Seek from file beginning */
			#define SEFILE_CURRENT 	SEEK_CUR    /**< Seek from current position */
			#define SEFILE_END 		SEEK_END    /**< Seek from file ending */
		#endif
	///@}
/** @}*/

/** \defgroup errorValues error values
 * @{
 */
	/** \name  Returned error values.
	 */
	///@{
		#define SEFILE_ENV_ALREADY_SET 		15
		#define SEFILE_ENV_WRONG_PARAMETER 	16
		#define SEFILE_ENV_MALLOC_ERROR		17
		#define SEFILE_ENV_NOT_SET			18
		#define SEFILE_SECTOR_MALLOC_ERR	19
		#define SEFILE_GET_FILEPOINTER_ERR	20
		#define SEFILE_HANDLE_MALLOC_ERR	21
		#define SEFILE_CLOSE_HANDLE_ERR		22
		#define SEFILE_CREATE_ERROR			23
		#define SEFILE_OPEN_ERROR 			24
		#define SEFILE_WRITE_ERROR			25
		#define SEFILE_SEEK_ERROR			26
		#define SEFILE_READ_ERROR			27
		#define SEFILE_ILLEGAL_SEEK			28
		#define SEFILE_FILESIZE_ERROR		29
		#define SEFILE_BUFFER_MALLOC_ERR	30
		#define SEFILE_FILENAME_DEC_ERROR	31
		#define SEFILE_FILENAME_ENC_ERROR	32
		#define SEFILE_DIRNAME_ENC_ERROR	33
		#define SEFILE_DIRNAME_DEC_ERROR	34
		#define SEFILE_DIRNAME_TOO_LONG		35
		#define SEFILE_MKDIR_ERROR			36
		#define SEFILE_LS_ERROR				37
		#define SEFILE_ENV_INIT_ERROR		38
		#define SEFILE_ENV_UPDATE_ERROR		39
		#define SEFILE_INTEGRITY_ERROR		40
		#define SEFILE_NAME_NOT_VALID		41
		#define SEFILE_TRUNCATE_ERROR		42
		#define SEFILE_DEVICE_SN_MISMATCH   43
		#define SEFILE_KEYID_NOT_PRESENT    44
		#define SEFILE_ALGID_NOT_PRESENT    45
		#define SEFILE_PATH_TOO_LONG        46
		#define SEFILE_SYNC_ERR             47
		#define SEFILE_SIGNATURE_MISMATCH   48
		#define SEFILE_RECRYPT_ERROR        49
	///@}
/** @}*/

/**
 * @defgroup Sector_Defines
 *  @{
 */
	/**
	 * \name Constant used to define sector structure.
	 */
	///@{
		#ifndef SEFILE_SECTOR_SIZE
		#define SEFILE_SECTOR_SIZE 			512    /**< Sector size. Use only power of 2. */
		#endif
		#define SEFILE_BLOCK_SIZE			B5_AES_BLK_SIZE				/**< Cipher block algorithm requires to encrypt data whose size is a multiple of this block size. */
		#define SEFILE_SECTOR_DATA_SIZE		(SEFILE_SECTOR_SIZE - B5_SHA256_DIGEST_SIZE) /**< The actual valid data may be as much as this, since the signature is coded on 32 bytes. */
		#define SEFILE_LOGIC_DATA			(SEFILE_SECTOR_DATA_SIZE-2) /**< The largest multiple of \ref SEFILE_BLOCK_SIZE that can fit in \ref SEFILE_SECTOR_DATA_SIZE */
		#define SEFILE_SECTOR_OVERHEAD 		(SEFILE_SECTOR_SIZE-SEFILE_LOGIC_DATA) /**< The amount of Overhead created by \ref SEFILE_SECTOR::len and \ref SEFILE_SECTOR::signature */
	///@}
/** @}*/

#define MAX_PATHNAME 256 /**< Maximum length for path in OS. */
typedef struct SEFILE_HANDLE* SEFILE_FHANDLE;  /**< Pointer to the data structure used by SEfile to wrap a traditional OS-dependent file descriptor taking into account the overhead of SEfile. */

#ifdef __cplusplus // these functions must be callable from C (SQLite)
extern "C" {
#endif

 /**
  * @brief This function computes the encrypted name of the file specified as path.
  * @param [in] path Absolute or relative path of a file (it must be plaintext, it cannot be a directory).
  * @param [out] enc_name Pre-allocated char array where the encrypted filename should be stored.
  * @param [out] encoded_length Length of the encrypted filename.
  * @return The function returns 0 in case of success. See \ref errorValues for error list.
  * @details The encrypted filename is the SHA-256 digest of the original name. It is expressed as a string of 64 characters (HEX format).
  */
uint16_t crypto_filename(char *path, char *enc_name, uint16_t *encoded_length);

/**
 * @defgroup SQLite Custom VFS wrappers
 *  @{
 */
	/**
	 * \name Wrappers of SEfile APIs for the SQLite DB engine.
	 * \details These APIs are C wrappers around the C++ APIs of the SEcure Database library. They are called automatically by the custom VFS of SQLite;
     * they are required by SEkey and by the SEcure Database library. They must not be used directly.
	 */
	///@{
		uint16_t c_sql_secure_open(char *path, SEFILE_FHANDLE *hFile, int32_t mode, int32_t creation);
		uint16_t c_sql_secure_write(SEFILE_FHANDLE *hFile, uint8_t * dataIn, uint32_t dataIn_len);
		uint16_t c_sql_secure_read(SEFILE_FHANDLE *hFile,  uint8_t * dataOut, uint32_t dataOut_len, uint32_t * bytesRead);
		uint16_t c_sql_secure_seek(SEFILE_FHANDLE *hFile, int32_t offset, int32_t *position, uint8_t whence);
		uint16_t c_sql_secure_truncate(SEFILE_FHANDLE *hFile, uint32_t size);
		uint16_t c_sql_secure_getfilesize(char *path, uint32_t * position);
		uint16_t c_secure_sync(SEFILE_FHANDLE *hFile);
		uint16_t c_secure_close(SEFILE_FHANDLE *hFile);
	///@}
/** @}*/

#ifdef __cplusplus
}
#endif

#endif
