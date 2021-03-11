/**
  ******************************************************************************
  * File Name          : environment.h
  * Description        : This is a header file required by any software using SEfile and/or SEkey.
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

/*! \file environment.h
 *  \brief This is a header file required by any application exploiting SEfile and/or SEkey.
 *  \author Fornero Matteo
 *  \date 25/11/2019
 */

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

#include "../sources/L1/L1.h"
#include "SEfile.h"

extern L1 *SEcube; /**< Global pointer to the L1 object used for L1 SEcube APIs (required by SEkey and SEfile). */
extern std::vector<std::unique_ptr<SEfile>> databases; /**< This contains the list of encrypted SQLite databases used by the software. Notice that at least one database is included (the db of SEkey) but you can add your own. */
extern bool SEkey_running; /**< Global flag that tells us if SEkey is running or not. It is set to true by sekey_start() and reset to false by sekey_stop(), it is used by the secure_key_check() function. */

#endif /* ENVIRONMENT_H_ */
