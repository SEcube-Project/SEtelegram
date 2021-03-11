/**
  ******************************************************************************
  * File Name          : SEcureDB.h
  * Description        : Header file for encrypted SQLite database (with SEfile).
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

/*! \file  SEcureDB.h
 *  \brief This header contains prototypes, constants and structures that are needed to implement the encrypted SQLite database (a.k.a. SEcure Database) based on SEfile.
 *  \author Fornero Matteo
 *  \date 25/11/2019
 */

#ifndef SECUREDB_H_
#define SECUREDB_H_

#include "SEfile.h"

#undef SEFILE_SQL_SECTOR_SIZE
#define SEFILE_SQL_SECTOR_SIZE 4096 /**< This is the size of the sector used by SEfile when the file itself contains a SQLite database. This value must be a power of 2 (512, 1024, 2048, 4096 recommended values).
 * If you change this value, you MUST also change the same value defined at the very beginning of the file sqlite3.c. */

/** This is the amount of data reserved to SQLite in the SEfile implementation customized to
 * work with SQLite. It must be a power of 2, the closest power of 2 which is smaller than
 * the dimension of the SEfile sector is the dimension of the sector itself divided by 2. */
#define SEFILE_SQL_LOGIC_DATA (SEFILE_SQL_SECTOR_SIZE/2)

/** This is the dimension of the len attribute in a SEfile sector.
 * It is an uint16_t value so it is 2 bytes. */
#define SEFILE_LEN_FIELD 2

/** This is a fixed-length padding that must be added to each SEfile sector to make it a multiple
 * of 16 (which is the block length of AES used for encryption by SEfile). This is required only
 * by the SEfile version customized to work with SQLite, it is not needed for the standard SEfile
 * implementation. The value is 14 because the actual SQLite data inside the sector are always a
 * power of 2 (so they are a multiple of 16) but we must also encrypt the len attribute which is
 * 2 bytes so we need other 14 bytes to reach a new size which is a multiple of 16. */
#define SEFILE_SQL_PADDING_LEN 14

/** This is the length of the overhead field that is included in each SEfile sector when SEfile is
 * used for SQLite (this field does not exists in standard SEfile). The overhead field is used as
 * "padding" to reach a sector dimension which is a power of 2 (because the sector itself must
 * include a field reserved for SQLite data which is a power of 2, but it must also include the
 * signature and the len attribute so we must go from the power of 2 reserved for SQLite data up
 * to the next power of two). */
#define SEFILE_SQL_OVERHEAD_LEN (SEFILE_SQL_SECTOR_SIZE - SEFILE_SQL_LOGIC_DATA - B5_SHA256_DIGEST_SIZE - SEFILE_LEN_FIELD - SEFILE_SQL_PADDING_LEN)

/** This is the amount of data which will be encoded/decoded by SEfile. Therefore it includes
 * the SQLite data, the len field and the padding used to reach a size that is multiple of
 * the basic encryption block size. */
#define SEFILE_SQL_SECTOR_DATA_SIZE (SEFILE_SQL_SECTOR_SIZE - B5_SHA256_DIGEST_SIZE - SEFILE_SQL_OVERHEAD_LEN)

/** This is the quantity of bytes of a sector which is not strictly reserved to the actual file content. */
#define SEFILE_SQL_SECTOR_OVERHEAD (SEFILE_SQL_SECTOR_SIZE - SEFILE_SQL_LOGIC_DATA)

#pragma pack(push,1)
/** @brief The SEFILE_SQL_SECTOR struct
 * This data struct is the actual sector organization for encrypted SQLite databases.
 * The total size should ALWAYS be equal to \ref SEFILE_SQL_SECTOR_SIZE.
 * The first sector is used to hold ONLY the header.
 * Thanks to the union data type, the developer can simply declare
 * a sector and then choose if it is the header sector or not.
 * This sector is similar to the original one but with higher overhead. */
struct SEFILE_SQL_SECTOR {
	union {
        SEFILE_HEADER header;               /**< See \ref SEFILE_HEADER .*/
        uint8_t data[SEFILE_SQL_LOGIC_DATA];   /**< In here it will be written the actual data.
         *   Since it is inside a union data type, the
         *   filename will be written from 32nd byte.
         */
    };
	uint8_t padding[SEFILE_SQL_PADDING_LEN]; // this is used to roundup the data to be encrypted (must be multiple of the block size)
    uint16_t len;                      /**< How many bytes are actually stored in this sector.*/
    uint8_t signature[32];             /**< Authenticated digest generated by the device*/
    uint8_t overhead[SEFILE_SQL_OVERHEAD_LEN]; /* This is used to make SEfile compatible with SQLite. */
    SEFILE_SQL_SECTOR();
};
#pragma pack(pop)

uint16_t securedb_get_secure_context(std::string& filename, std::string *keyid, uint16_t *algo); /**< @brief Same as get_secure_context() but for encrypted SQLite databases. */
uint16_t securedb_ls(std::string& path, std::vector<std::pair<std::string, std::string>>& list, L1* SEcubeptr); /**< @brief Same as secure_ls() but for encrypted SQLite databases. */
uint16_t securedb_decrypt_filename(std::string& path, char *filename, L1 *SEcubeptr); /**< @brief Same as decrypt_filename() but for encrypted SQLite databases. */
uint16_t securedb_recrypt(std::string& path, uint32_t key, L1 *SEcubeptr); /**< @brief Same as secure_recrypt() but for encrypted SQLite databases. */
size_t securedb_pos_to_cipher_block(size_t current_position); /**< @brief Same as pos_to_cipher_block() but for encrypted SQLite databases. */
uint16_t securedb_secure_getfilesize(char *path, uint32_t * position); /**< @brief Same as secure_getfilesize() but for encrypted SQLite databases. */

#endif
