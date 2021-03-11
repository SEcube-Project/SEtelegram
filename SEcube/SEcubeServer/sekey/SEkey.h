/**
  ******************************************************************************
  * File Name          : SEkey.h
  * Description        : SEkey library header.
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

/*! \file  SEkey.h
 *  \brief This file includes everything about SEkey.
 *  \author Fornero Matteo
 *  \date 22/11/2019
 */

#ifndef SEKEY_H_
#define SEKEY_H_

#include "../sefile/SEfile.h"
#include "../sqlite/sqlite3.h"

#define PINLEN 32 /**< @brief Length (bytes) of the PIN used to login as user or admin to the SEcube. */
#define AES256KEYLEN 32 /**< @brief Length of an AES-256 key expressed in bytes. */
#define IDLEN 11 /**< @brief Maximum length expected (in bytes) for a generic ID (could be a key, a user or a group). The value is 11 because each ID must have 1 literal at the beginning followed by up to 10 numbers. */
#define NAMELEN 100 /**< @brief This is the maximum length accepted for a name or label (i.e. the username, the label of a group or the label of a key). */
#define TRY_LIMIT 5 /**< @brief Maximum number of attempts updating SEkey in user mode. If the limit is reached and the update failed, recovery will be needed. */
#define UPDATE_RECORD_HEADER_LEN 11 /**< @brief Length of header of each update record in the update, init or recovery file. 1 byte for the type, 8 bytes for the counter, 2 bytes for the length. */

/** @brief Record type identifiers for SEkey update files. */
enum update_record_type {
	DELETE_USER_FROM_GROUP, /**< Request to delete a user from a group. */
	DELETE_USER, /**< Request to delete a user from SEkey. */
	KEY_DATA, /**< Request to store a new key inside the SEcube flash memory. */
	SQL_QUERY, /**< Request to execute a SQL query on the database. */
	DELETE_GROUP /**< Request to delete a group from SEkey. */
};

/** @brief Error codes returned by functions of SEkey. */
enum sekey_error {
	SEKEY_OK = 0, /**< Function completed correctly. */
	SEKEY_ERR = 1, /**< Generic error. SEkey database unchanged. */
	SEKEY_FILE_FOUND = 2, /**< File found. */
	SEKEY_FILE_NOT_FOUND = 3, /**< File not found. */
	SEKEY_ERR_AUTH = 4, /**< Requested operation not allowed with current SEkey mode (admin or user). */
	SEKEY_USER_GROUP_DUP = 5, /**< User already belonging to the group. */
	SEKEY_KEY_DUP = 6, /**< Key already stored in SEkey. */
	SEKEY_GROUP_NOT_FOUND = 7, /**< No such group inside SEkey. */
	SEKEY_GROUP_DUP = 8, /**< Group already stored in SEkey. */
	SEKEY_USER_DUP = 9, /**< User already stored in SEkey. */
	SEKEY_USER_NOT_FOUND = 10, /**< No such user inside SEkey. */
	SEKEY_KEY_NOT_FOUND = 11, /**< No such key inside SEkey. */
	SEKEY_ERR_PARAMS = 12, /**< Invalid input parameters. */
	SEKEY_UNCHANGED = 13, /**< Error. SEkey database unchanged. */
	SEKEY_RESTART = 14, /**< Error while executing commit or rollback on the database. Restart the application to go back to coherent database content. */
	SEKEY_CORRUPTED = 15, /**< SEkey database corrupted. */
	SEKEY_REPROG = 16, /**< Error while initializing a SEcube. Device erase needed. */
	SEKEY_RESTART_REPROG = 17, /**< Error while initializing a SEcube. Device erase needed and SEkey host application restart required. */
	SEKEY_BLOCKED = 18, /**< Impossible to update SEkey. SEkey blocked until update is completed. */
	SEKEY_COMMON_GROUP_NOT_FOUND = 19, /**< Returned when the application asks to SEkey to return the key to encrypt data for a set of users but there is no common groups among those users. The application must check for this
	error and, in case this error is returned, proceed to request a key to encrypt data in a unicast manner (1 to 1) instead of multicast (1 to many). */
	SEKEY_UNSUPPORTED = 20, /**< Returned when a certain feature is not yet supported by the system (i.e. requesting a key with a type different from symmetric data encryption). */
	SEKEY_INVALID_KEY = 21, /**< The key is not valid (i.e. does not exist or has invalid attributes). */
	SEKEY_INACTIVE_KEY = 22, /**< The key is not active, so can't be used for encryption. */
	SEKEY_COMPROMISED_KEY = 23, /**< The key is in compromised status. */
	SEKEY_DESTROYED_KEY = 24, /**< The key is in destroyed status. */
	SEKEY_DEACTIVATED_KEY = 25, /**< The key is in deactivated status. */
	SEKEY_PREACTIVE_KEY = 26, /**< The key is in preactive status. */
	SEKEY_SUSPENDED_KEY = 27 /**< The key is in suspended status. */
};

/** @brief Possible status assumed by a key. There are specific rules for status transition, see check_key_transition_validity() for more informations. */
enum class se_key_status {
	statusmin = 0, /**< Lower boundary. This status is not valid, it is used only to check if the actual status value is within the boundaries. */
	preactive = 1, /**< Default key status (automatically set on key generation). */
    active = 2, /**< The key can be used for encryption and decryption. */
    suspended = 3, /**< The key may be used for decryption, it may also be reactivated in future. */
    deactivated = 4, /**< The key can be used only for decryption. */
    compromised = 5, /**< The key must not be used for encryption. Files encrypted with this key must be decrypted and re-encrypted as soon as possible with a new key. */
    destroyed = 6, /**< The key is not stored anymore in the KMS. Only the metadata are kept. */
	statusmax = 7 /**< Upper boundary. This status is not valid, it is used only to check if the actual status value is within the boundaries. */
};

/** @brief Possible key types. Notice that only symmetric_data_encryption is supported, other types are listed here in case
 *  someone wanted to expand the features of the system. */
enum class se_key_type {
	typemin = 100,
	private_signature = 101,
	public_signature_verification = 102,
	symmetric_authentication = 103,
	private_authentication = 104,
	public_authentication = 105,
	symmetric_data_encryption = 106,
	symmetric_key_wrapping = 107,
	symmetric_RGB = 108,
	symmetric_key_derivation = 109,
	private_key_transport = 110,
	public_key_transport = 111,
	symmetric_key_agreement = 112,
	private_static_key_agreement = 113,
	public_static_key_agreement = 114,
	private_ephemeral_key_agreement = 115,
	public_ephemeral_key_agreement = 116,
	symmetric_authorization = 117,
	private_authorization = 118,
	public_authorization = 119,
	typemax = 120
};

/** @brief Used to identify the different types of files used by the update mechanism of SEkey.
 * The hierarchy is the same, where the init file is the most important update file
 * (the first to be processed), then there is the recovery file and finally the standard
 * update file (used when SEkey is operating normally without errors). */
enum filetype {
	INIT = 1, /**< Used to specify mode = init file in the open_update_file() function. */
	RECOVERY = 2, /**< Used to specify mode = recovery file in the open_update_file() function. */
	NORMAL = 3 /**< Used to specify mode = normal update file in the open_update_file() function. */
};

/** @brief Structure used to enclose all elements required by the APIs to initialize a user device inside SEkey. */
typedef struct userdata_{
	std::array<uint8_t, L0Communication::Size::SERIAL> sn; /**< SEcube serial number */
	std::string uid; /**< user id */
	std::string uname; /**< username */
	uint32_t k1; /**< ID of symmetric key to encrypt user updates */
	uint32_t k2; /**< ID of symmetric key for key wrapping inside updates */
	uint32_t algo; /**< algorithm for encryption of updates */
	uint32_t klen; /**< length of keys */
	std::string query; /**< SQL query to be executed by the user */
	std::shared_ptr<uint8_t[]> k1_data; /**< content of k1 */
	std::shared_ptr<uint8_t[]> k2_data; /**< content of k2 */
	std::shared_ptr<uint8_t[]> wcard_key; /**< content of wildcard key (used to send an update to a former member of SEkey)*/
}userdata;

/** @brief Simlpe struct used to store the details about the SEkey user which is using the application.
 * ID and name are not really user when SEkey is in admin mode, they are used when SEkey is in
 * user mode. The device serial number instead is always used and is crucial for the correct execution
 * of the SEkey KMS. */
typedef struct userinfo_{
	std::string userid;
	std::string username;
	std::string device_sn;
} userinfo;

/** @brief The user class is used to model a user object. User's attributes are private but methods
 * are available to retrieve them. */
class se_user{
private:
	std::string id; /**< ID of the user */
	std::string name; /**< Name of the user. */
	std::string sn; /**< Serial number of the SEcube of the user. */
	std::string user_pin; /**< The PIN of the SEcube to login with low privilege (user mode, always disclosed to the user by the administrator). */
	std::string admin_pin; /**< The PIN of the SEcube to login with high privilege (admin mode, never disclosed to the user by the administrator). */
	std::string algorithm; /**< Algorithm used by the administrator to encrypt the SEkey updates for this user. */
	uint32_t k1; /**< ID of the key used to encrypt updates. */
	uint32_t k2; /**< ID of the key used to encrypt keys to be written inside the updates. */
	uint32_t init; /**< Flag that tells if the user SEcube is initialized or not. */
	int64_t update_cnt; /**< Ascending identifier of records written inside SEkey update files. */
	std::vector<std::string> groups; /**< List of groups to which the user belongs. */
public:
	se_user(): k1(0), k2(0), init(0), update_cnt(0){};
	se_user(std::string& _id, std::string& _name): id(_id), name(_name), k1(0), k2(0), init(0), update_cnt(0){};
	se_user(std::string& user_id, std::string& user_name, std::string& serialnumber, std::string& userpin, std::string& adminpin, uint32_t k1, uint32_t k2, uint32_t algo, uint32_t init_flag, int64_t cnt);
	void set_id(std::string& new_id); /**< Set the ID of the user. */
	std::string& get_id(); /**< Retrieve the ID of the user. */
	std::string& get_name(); /**< Retrieve the name of the user. */
	void set_name(std::string& new_name); /**< Set the name of the user. */
	void add_group(std::string& group); /**< Add a group to the list of the groups to which the user belongs. */
	void print_user_details(std::ofstream& sekey_log); /**< used for debugging purpose */
	std::string& get_sn(){return this->sn;};
};

/** @brief The policy class is used to model a security policy adopted by a group.
 * A policy for a group has 3 features: a maximum number of keys for the group,
 * a common algorithm for all keys belonging to the group, a default cryptoperiod
 * common to all keys belonging to the group. */
class group_policy{
private:
	uint32_t max_keys; /**< Maximum number of keys for the group. */
	uint32_t algorithm; /**< Algorithm used by all keys of this group. */
	uint32_t default_cryptoperiod; /**< Liveness of the keys of this group. */
	friend class se_group;
public:
	group_policy(uint32_t maxkeys, uint32_t algo, uint32_t cryptoperiod);
	group_policy();
	uint32_t get_max_keys();
	uint32_t get_algorithm();
	uint32_t get_default_cryptoperiod();
	void set_max_keys(uint32_t maxkeys);
	void set_default_cryptoperiod(uint32_t cryptoperiod);
	void set_algorithm(uint32_t algo);
	bool isvalid(); /**< A policy is valid if the algorithm is valid, the max number of keys is greater than 0 and the default cryptoperiod is greater than 0. */
};

/**
 * @brief The key class is used to model a key object. A key has several properties which
 * are kept private (id, name, owner, status, algorithm, length, generation time,
 * activation time, expiration time). Public methods are available to retrieve
 * the values of private properties.
*/
class se_key {
private:
	std::string id; /**< id of the key */
	std::string name; /**< label of the key */
	std::string owner; /**< the owner of a key is always a group */
	se_key_status status; /**< status (i.e. preactive) */
	se_key_type type; /**< the type of the key (i.e. symmetric data encryption) */
	uint32_t algorithm; /**< algorithm (must be the same specified by the policy of the group owner of the key) */
	uint32_t length; /**< given the algorithm, the length of the key (expressed in bits) is automatically determined */
	time_t generation; /**< time at which the key was created */
	time_t activation; /**< time at which the key was activated */
	time_t expiration; /**< time at which the key expired or was supposed to expire (deactivation is always less or equal to the expiration time) */
	time_t deactivation; /**< time at which the key was deactivated */
	time_t compromise; /**< time at which the key was compromised (0 if it is not compromised) */
	time_t destruction; /**< time at which the key was destroyed (0 if it is not destroyed) */
	time_t suspension; /**< last time at which the key was suspended (0 if it is not suspended) */
	time_t cryptoperiod; /**< time span during which a key can be used for encryption and decryption, if 0 the cryptoperiod of the owner is inherited */
public:
	se_key(): status(se_key_status::preactive), type(se_key_type::symmetric_data_encryption), algorithm(L1Algorithms::Algorithms::AES_HMACSHA256), length(0), generation(0),
		activation(0), expiration(0), deactivation(0), compromise(0), destruction(0), suspension(0), cryptoperiod(0){};
	se_key(std::string& key_id, uint32_t algo, uint32_t key_length, time_t act, time_t exp);
	se_key(std::string& key_id, std::string& key_name, std::string& key_owner, se_key_status key_status, uint32_t key_algo, uint32_t key_length, time_t gen, time_t act,
			time_t exp, time_t crypto, time_t deactivation, se_key_type key_type, time_t compr, time_t destr, time_t susp);
	se_key& operator= (const se_key& key);
	std::string& get_id();
	se_key_status get_status();
	bool safer(se_key& chosen); /**< Determines if the current key is safer than another key. */
	void print_key_details(std::ofstream& sekey_log); // used for debugging purpose
};

/** @brief Implement the concept of group inside SEkey.
 * Class members are kept private and suitable getter/setter methods are provided. */
class se_group{
private:
	std::string id; /**< ID of the group. */
	std::string name; /**< Name of the group. */
	uint32_t users_counter; /**< The number of users currently in the group. */
	uint32_t keys_counter; /**< The number of keys currently in the group. */
	group_policy policy; /**< The policy of the group. */
	std::vector<se_user> users_list; /**< Not used at the moment. */
	std::vector<se_key> keys_list; /**< Not used at the moment. */
public:
	se_group(std::string& groupid, std::string& groupname, group_policy gpolicy);
	se_group(): users_counter(0), keys_counter(0){};
	std::string& get_id();
	void set_id(std::string& new_id);
	std::string& get_name();
	void set_name(std::string& new_name);
	uint32_t get_users_counter();
	void set_users_counter(uint32_t cnt);
	uint32_t get_keys_counter();
	void set_keys_counter(uint32_t cnt);
	uint32_t get_keys_maxnumber();
	void set_keys_maxnumber(uint32_t max);
	uint32_t get_keys_algorithm();
	void set_keys_algorithm(uint32_t algo);
	uint32_t get_keys_cryptoperiod();
	void set_keys_cryptoperiod(uint32_t cryptoperiod);
	void print_group_details(std::ofstream& sekey_log); // used for debugging purpose
};

/** @brief Handy RAII wrapper for sqlite3_stmt which requires call to sqlite3_finalize to avoid resource leakage.
 *  See the SQLite documentation for more informations about sqlite3_stmt. */
class statement{
private:
	sqlite3_stmt *stmt; /**< Pointer to the statement as required by SQLite. */
public:
	/** The constructor will set the statement pointer to NULL, the statement will then be allocated by an explicit call to sqlite3_prepare(). */
	statement(){ this->stmt = nullptr; };
	/** Destructor involving sqlite3_finalize() to avoid memory leaks. */
	~statement(){ sqlite3_finalize(this->stmt); this->stmt = nullptr; };
	 /** Returns the pointer to the SQLite statement. */
	sqlite3_stmt *getstmt(){ return this->stmt; };
	 /** Returns the pointer to the pointer of the SQLite statement, implies finalization of previous pointer if not NULL (used for cyclig usage of same statement object). */
	sqlite3_stmt **getstmtref(){ sqlite3_finalize(this->stmt); return &(this->stmt); };
	/** Wrapper for sqlite3_finalize() SQLite API. */
	void finalize(){ sqlite3_finalize(this->stmt); };
};

/**
 * @defgroup sekey_apis
 *  @{
 */
	/**
	 * \name APIs of SEkey
	 * \details These are the APIs available to use SEkey.
	 */
	///@{

		/** @brief API to start the SEkey KMS.
		* @param[in] l0 L0 object created by the caller.
		* @param[in] l1ptr The pointer to the L1 object created iby the caller.
		* @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		* @details This API must be called before any other API of SEkey. It is used to initialize SEkey,
		* in particular it will search for an existing database dedicated to SEkey and it will create the database
		* in case it can't be found on the SEcube's SD card. Any pending SEkey database transaction left on disk
		* by previous crashes of the application, power loss or unexpected event will be rolled back. When opening
		* the database, an integrity check of the database will be performed. Upon completion, the function will
		* force a SEcube flash maintenance routine (to clear any data inside the device flash which is not needed
		* by SEkey) and it will also force the SEkey update when in user mode.  Any exception will result in the
		* function returning an error, when this API does not return SEKEY_OK there is no need to call sekey_stop(). */
		int sekey_start(L0& l0, L1 *l1ptr);

		/** @brief API to stop the SEkey KMS.
		 * @return This API always returns SEKEY_OK, errors are ignored, no throw.
		 * @details Notice that, after executing this API, all other APIs based on SEkey will stop working because
		 * SEkey itself has been shut down. Must be called before closing the main application. */
		int sekey_stop();

		/** @brief Initialize the SEcube of the administrator of SEkey.
		 * @param [in] l1 The L1 object created by the caller.
		 * @param [in] pin A list of PINs that can be used to login to the SEcube prior to its initialization.
		 * @param [in] userpin The user PIN to be set on the SEcube.
		 * @param [in] adminpin The admin PIN to be set on the SEcube.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This API must be called only once in order to initialize the SEcube of the administrator of SEkey.
		 * For the second parameter, in general is enough to pass a vector with only one PIN made of 32 bytes initialized at zero (this is the
		 * default PIN of every SEcube prior to initialization). The third and fourth parameters are the new PINs that will be set on the device,
		 * notice that they are not saved anywhere so if you forget them you will not be able to recover them. */
		int sekey_admin_init(L1& l1, std::vector<std::array<uint8_t, L1Parameters::Size::PIN>>& pin, std::array<uint8_t, L1Parameters::Size::PIN>& userpin, std::array<uint8_t, L1Parameters::Size::PIN>& adminpin);

		/** @brief API to initialize the SEcube device of a SEkey user. Admin only.
		 * @param [in] uid The ID of the user to initialize.
		 * @param [in] userpin The PIN to be set on the SEcube of the user to perform the login with user privilege.
		 * @param [in] adminpin The PIN to be set on the SEcube of the user to perform the login with admin privilege.
		 * @param [in] pin The list of PINs that can be used to login as admin on the SEcube of the user before it gets initialized (usually a single PIN with 32 bytes at zero is enough).
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * This API must be called after the sekey_add_user() function. It does not matter how much time after you call it, as long as you do it before physically giving the SEcube to
		 * the SEkey user. For Linux users: when you connect the SEcube of the user to the host computer, remember to mount the SEcube otherwise this API might fail. */
		int sekey_init_user_SEcube(std::string& uid, std::array<uint8_t, L1Parameters::Size::PIN>& userpin, std::array<uint8_t, L1Parameters::Size::PIN>& adminpin, std::vector<std::array<uint8_t, L1Parameters::Size::PIN>>& pin);

		/** @brief Add a new user to SEkey. Available only for the administrator.
		 * @param [in] user_id The ID of the new user.
		 * @param [in] username The name of the new user.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details When a new user is created, it does not belong to any group. After calling this function,
		 * the sekey_init_user_SEcube() should be called to initialize also the SEcube of the user. */
		int sekey_add_user(std::string& user_id, std::string& username);

		/** @brief Delete a user from SEkey; the user will not be able to use the key management system anymore. Available only for the administrator.
		 * @param [in] userID The ID of the user to be deleted from SEkey.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This function will completely delete the user from SEkey. Every information related to that user will be deleted from every SEcube
		 * involved in SEkey (i.e. the SEcube of the administrator or the SEcube of another user who was associated to the deleted user by means of a
		 * common group). The SEcube of the deleted user will receive a special update that will erase completely its keys and the informations related
		 * to SEkey. The ID of the deleted user becomes available again. Any exception will result in the function returning an error. */
		int sekey_delete_user(std::string& userID);

		/** @brief Add an existing user to an existing group. Available only for the administrator.
		 * @param [in] userID The ID of the user to be added. Each user ID is unique
		 *  in SEkey but this API can be called multiple times for the same
		 *  user ID provided that the group ID is different. In fact the internal table of the
		 *  users managed by SEkey uses the pair (user ID, group ID) as primary key.
		 * @param [in] groupID The ID of the group to which the user must be added. This parameter
		 *  must be an existing group.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details The SEcube of the involved user will receive all informations about the new group, the users of that group
		 * and the keys of that group. The other users of that group will receive informations about the new user.
		 * Any exception will result in the function returning an error. */
		int sekey_add_user_group(std::string& userID, std::string& groupID);

		/** @brief Delete a user from a group. All keys of that group will be deleted from the SEcube of the user. Available only for the administrator.
		 * @param [in] user_id The ID of the user to be deleted from SEkey.
		 * @param [in] group_id The ID of the group from which the user must be deleted.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details The specified user will simply stop belonging to the specified group. Nothing else will change from the administrator point of
		 * view (except for obvious things like the number of users in the group which will be decremented). From the user point of view, if the
		 * user to be deleted from the group is the very same user logged in to the SEcube, all the keys of that group will be deleted from SEkey.
		 * If the user is not the same, the user will simply be deleted from the list of users known by the current user, if the specified group
		 * is the only one in common. Any exception will result in the function returning an error. */
		int sekey_delete_user_group(std::string& user_id, std::string& group_id);

		/** @brief Change the name of a user. Available only for the administrator.
		 * @param [in] userID The ID of the user whose name should be changed.
		 * @param [in] newname The new name of the user.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.*/
		int sekey_user_change_name(std::string& userID, std::string& newname);

		/** @brief Retrieve the details about all users of SEkey.
		 * @param [in] users Pointer to a vector of se_user objects.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This API will fill the vector passed by reference with user objects, each one containing the details about one user of SEkey.
		 * Be careful when using this API because it could potentialy use a lot of RAM memory. This API is useful when the higher level
		 * application must perform particular analysis filtering the data. Is impossible to write an API for each possible filter that might be
		 * needed by the higher level application (e.g. only users whose name is "Paul") therefore this API is intended as an easy way to provide
		 * to the higher level application all the data it needs in order to perform further analysis. */
		int sekey_user_get_info_all(std::vector<se_user> *users);

		/** @brief Retrieve the details about a single user.
		 * @param [in] userid The ID of the users we want to search for in the KMS.
		 * @param [in] user Pointer to a se_user object that will contain the details of the user.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This function works similarly to the sekey_user_get_info_all() API. The main difference is that, in this case, the caller is
		 * supposed to pass the ID of the user he is interested in. Therefore this API is used when the caller wants to find the details about a single user. */
		int sekey_user_get_info(std::string& userid, se_user *user);

		/** @brief API to add a group to SEkey. Available only for the administrator.
		 * @param [in] groupID The ID of the group to be added to SEkey.
		 * @param [in] group_name The name of the group to be added to SEkey.
		 * @param [in] policy The policy of the group. See group_policy object for more informations.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This API adds an empty group to SEkey. The group is not associated to any key by default. */
		int sekey_add_group(std::string& groupID, std::string& group_name, group_policy policy);

		/** @brief API to delete a group from SEkey. Available only for the administrator.
		 * @param [in] groupID The ID of the group to delete.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details Delete a group from SEkey. This API basically deletes every reference to the group with the specified ID from the system. This means that
		 * after calling this function, SEkey will not have any group with that ID and the users who belonged to the group will not belong to it anymore.
		 * The keys associated to the delete group will be deactivated and their owner will be set to "zombie". The zombie keys may still be used by the
		 * administrator to decrypt old data. If they are not needed anymore, the administrator can delete them changing their status to destroyed. */
		int sekey_delete_group(std::string& groupID);

		/** @brief Change the name of a group. Available only for the administrator.
		 * @param [in] groupID The ID of the group whose name must be changed.
		 * @param [in] newname The new name of the group.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise. */
		int sekey_group_change_name(std::string& groupID, std::string& newname);

		/** @brief Change the maximum number of keys that a group can be associated to. Available only for the administrator.
		 * @param [in] groupID The group whose maximum number of key should be modified.
		 * @param [in] maxkeys The maximum number of keys.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details The new value can't be lower than the current number of keys. */
		int sekey_group_change_max_keys(std::string& groupID, uint32_t maxkeys);

		/** @brief Change the default cryptoperiod of a specific group. Available only for the administrator.
		 * @param [in] groupID The ID of the group to modify.
		 * @param [in] cryptoperiod The new value for the default cryptoperiod
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details API to change the default cryptoperiod of a group. The default cryptoperiod of the group is used for keys owned by the group
		 * when their specific cryptoperiod is set to 0 or when it is higher than the default cryptoperiod itself. */
		int sekey_group_change_default_cryptoperiod(std::string& groupID, uint32_t cryptoperiod);

		/** @brief Retrieve the details about a single group (keys and users excluded).
		 * @param [in] groupID The ID of the groups we want to search for in the KMS.
		 * @param [in] group Pointer to a se_group object that will contain the details of the group.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This function works similarly to the sekey_group_get_info_all() API. The main difference is that, in this case, the caller is supposed to pass the ID of the group he is interested in.
		 * Therefore this API is used when the caller wants to find the details about a single group. */
		int sekey_group_get_info(std::string& groupID, se_group *group);

		/** @brief Retrieve the details about all groups of SEkey (keys and users excluded).
		 * @param [in] groups Pointer to a vector of se_group objects.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This API will fill the vector passed by reference with group objects, each one containing the details about one group of the SEkey KMS.
		 * Be careful when using this API because it could potentially use a lot of RAM memory. This API is useful when the higher level application must perform particular
		 * analysis filtering the data. Is impossible to write an API for each possible filter that might be needed by the higher level application (e.g. only groups
		 * whose algorithm is aes-256) therefore this API is intended as an easy way to provide to the higher level application all the data it needs in order to perform
		 * further analysis. */
		int sekey_group_get_info_all(std::vector<se_group> *groups);

		/** @brief Add a key to SEkey. Available only for the administrator.
		 * @param [in] key_id ID of the key to be added.
		 * @param [in] key_name Name of the key to be added.
		 * @param [in] ID of the group to which the key belongs.
		 * @param [in] cryptoperiod The time span during which a key will be usable for encryption and decryption. Insert 0 if you want to inherit the cryptoperiod of the group owner of the key, insert any other value for a custom cryptoperiod.
		 * @param [in] keytype The type of the key. Currently only se_key_type::symmetric_data_encryption is supported.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details API to add a key to SEkey. The key must belong to a group, therefore the caller must provide the ID of the group which owns the key to be added (the key will be added only to that group).
		 * All other metadata of the key (i.e. the algorithm, the length, the generation time etc.) are computed automatically (i.e. the algorithm is automatically given by the group policy and the length is automatically given by the algorithm). */
		int sekey_add_key(std::string& key_id, std::string& key_name, std::string& key_owner, uint32_t cryptoperiod, se_key_type keytype);

		/** @brief Activate a key. Available only for the administrator.
		 * @param [in] key_id The ID of the key to activate.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This API activates a key. In case of invalid key ID, invalid transition (i.e. from deactivated to active) or missing key the API will return an error. On activation, also the expiration time of the
		 * key is computed (sum of generation time and cryptoperiod). The activation time is not updated if the key is activated multiple times (i.e. activate, suspend, activate again). */
		int sekey_activate_key(std::string& key_id);

		/** @brief Change the status of a key. Available only for the administrator.
		 * @param [in] key_id The ID of the key whose status should be changed.
		 * @param [in] status The new status of the key.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This API changes the status of a key but it must not be used to activate a key (which means changing the status to active) because the transition to the active
		 * status implies additional constraints and is handled by the sekey_activate_key() API. If the new status of the key is not compatible with the current status, an error is returned. */
		int sekey_key_change_status(std::string& key_id, se_key_status status);

		/** @brief Change the name of a key. Available only for the administrator.
		 * @param [in] key_id The ID of the key whose name should be changed.
		 * @param [in] key_name The new name of the key.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise. */
		int sekey_key_change_name(std::string& key_id, std::string& key_name);

		/** @brief Retrieve the details about a single key.
		 * @param [in] key_id The ID of the key to search for.
		 * @param [out] key Pointer to a se_key object that will contain the details of the key.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This function works similarly to the sekey_key_get_info_all() API. The main difference is that, in this case, the caller is supposed to pass the ID of the key he is interested in.
		 * Therefore this API is used when the caller wants to find the details about a single key. */
		int sekey_key_get_info(std::string& key_id, se_key *key);

		/** @brief Retrieve the details about all keys of SEkey.
		 * @param [out] keys Pointer to a vector of se_key objects.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This API will fill the vector passed by reference with key objects, each one containing the details about one key of the SEkey KMS.
		 * Be careful when using this API because it could potentially use a lot of RAM memory. This API is useful when the higher level application must perform particular
		 * analysis filtering the data. Is impossible to write an API for each possible filter that might be needed by the higher level application (e.g. only keys
		 * whose expiration time is in a specific interval and whose algorithm is aes-256) therefore this API is intended as an easy way to provide to the higher level
		 * application all the data it needs in order to perform further analysis. */
		int sekey_key_get_info_all(std::vector<se_key> *keys);

		/** @brief Find a suitable key to encrypt data given a couple of users source-destination.
		 * @param [out] chosen_key ID of the best key to be used given the specified parameters.
		 * @param [in] source_user_id User ID of the sender.
		 * @param [in] dest_user_id User ID of the receiver.
		 * @param [in] keytype The type of the key. Currently only se_key_type::symmetric_data_encryption is supported.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details The returned key ID is the ID of the best key given these rules: the key is active, the key belongs to the smallest group(s) in common between the users,
		 * the key uses the best encryption algorithm. If more than one key with these requirements is found, the most recent key is chosen assuming that data encrypted
		 * with that key have been observed for a shorter amount of time than other messages encrypted with a key which is older. If activation times are equal, the key with the lower ID is chosen. */
		int sekey_find_key_v1(std::string& chosen_key, std::string& dest_user_id, se_key_type keytype);

		/** @brief Find a suitable key to encrypt data to be delivered from a single user to an entire group.
		 * @param [out] chosen_key ID of the best key to be used given the specified group.
		 * @param [in] source_user_id User ID of the sender.
		 * @param [in] group_id Group ID of the receiver.
		 * @param [in] keytype The type of the key. Currently only se_key_type::symmetric_data_encryption is supported.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This API is intended to be used only when the destination of an operation is an entire group. It searches the most secure key that is active and belongs to the
		 * destination group.*/
		int sekey_find_key_v2(std::string& chosen_key, std::string& group_id, se_key_type keytype);

		/** @brief Find a suitable key to encrypt data given a sender and multiple recipients.
		 * @param [out] chosen_key ID of the best key to be used given the users specified.
		 * @param [in] source_user_id User ID of the sender.
		 * @param [in] dest_user_id IDs of the recipients.
		 * @param [in] keytype The type of the key. Currently only se_key_type::symmetric_data_encryption is supported.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This function works with the same rules of sekey_find_key_v1(). Notice that if there is not a group to which all users involved belong (sender and receivers)
		 * an error is returned because is not possible to identify a key that can be used to encrypt data only once for all the recipients. In conclusion, this function behaves
		 * as sekey_find_key_v2() but you do not need to provide an entire group as destination, you can provide a subset of his members. */
		int sekey_find_key_v3(std::string& chosen_key, std::vector<std::string>& dest_user_id, se_key_type keytype);

		/** @brief Retrieve the content of the logfile associated to a specific SEcube.
		 *  @param [in] sn The serial number of the SEcube whose logfile must be retrieved. If NULL the current SEcube is considered.
		 *  @param [out] output A string containing the content of the logfile. */
		int sekey_readlog(std::string* sn, std::string& output);
	///@}
/** @}*/

/**
 * @defgroup sekey_other_functions
 *  @{
 */
	/**
	 * \name Other functions of SEkey
	 * \details These functions are used by SEkey for internal purposes but they can also be used by higher levels if needed. Always check the documentation and the source code before using them.
	 */
	///@{

/** @brief SEkey interface to retrieve the timestamp to be set for any KMS need.
 *  This API should be customized in order to meet the security requirements of the environment integrating the SEcube.
 *  The basic version simply retrieves the current system time, a customized version may require to get the time from
 *  an authoritative time server. */
time_t sekey_gettime();

/** @brief Synchronize the data of SEkey stored on the SEcube of the user with the data stored on the SEcube of the administrator. Available only for the users.
 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
 * @details This function updates the data of SEkey stored inside the SEcube of a user fetching the latest updates prepared for him by the administrator.
 * If the update fails the KMS for the user is blocked until the update is completed or a full recovery of the data is performed. */
int sekey_update_userdata();

/** @brief Check for expired keys inside SEkey. Expired keys which are still flagged as active will be deactivated.
 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
 * This API will be run automatically by many APIs of the KMS and of SEfile; it can be used also inside the higher level application if needed. */
int sekey_check_expired_keys();

/** @brief Explicitly request to SEkey to execute the recovery procedure for a specific user, given his serial number. Available only for the administrator.
 * @param [in] user_id The ID of the user who needs to recovery his database.
 * @param [in] serial_number The serial number assigned to the SEcube of the user.
 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
 * This API is useful when the user requests to the administrator a recovery, possibly because the user is not able to complete successfully the SEkey update.
 * So the administrator can satisfy the request of the specific user running this API. Notice that this API has been written in order to be called explicitly
 * by the higher level application which is expected to handle the communication between users and administrator (out of bound, with a server in the middle,
 * a message broker or something else). */
int sekey_recovery_request(std::string& user_id, std::string& serial_number);

/** @brief Convert an integer (epoch time) to the local time as readable string.
 * @param [in] t The epoch time to be converted.
 * @return The string with the epoch time converted to local time.*/
std::string epoch_to_localtime(time_t t);

/** @brief Convert an integer to number of days, hours, minutes and seconds into a readable string.
 * @param [in] n The integer to be converted into days, hours, minutes, seconds format.
 * @return The string with the integer converted to the required format. */
std::string cryptoperiod_to_days(uint32_t n);

/** @brief Wrapper around stoul() function.
 * @param [in] s The string to convert to integer.
 * @return The string converted into the corresponding integer if the integer fits in 32 bits unsigned.
 * @details This wrapper is used to avoid stoul() returning values bigger than UINT32_MAX. If the converted
 * value does not fit in 32 bits, this function throws an out of range exception.  */
uint32_t stoul_wrap(std::string& s);

/** @brief Convert a key status to the corresponding string. May throw exceptions. */
std::string statusmap(se_key_status s);

/** @brief Convert a key type to the corresponding string. */
std::string keytypemap(se_key_type t);

/** @brief Map an algorithm (expressed as integer) to the corresponding algorithm expressed as string.
 * @param [in] algo The algorithm to be mapped.
 * @return The string of the corresponding algorithm. May throw exceptions. */
std::string algomap(uint32_t algorithm);

/** @brief Return the length (in byte) of the key, given the algorithm. Return 0 if algorithm is unknown. */
uint32_t algolen(uint32_t algorithm);

uint32_t keyIDclass(uint32_t id);
	///@}
/** @}*/

/**
 * @defgroup sekey_internal_functions
 *  @{
 */
	/**
	 * \name Internal functions of SEkey
	 * \details These functions are used by SEkey for internal purposes so they should not be called expliticly.
	 */
	///@{

		/** @brief Insert the string passed as parameter in the SEkey logfile of the current user or of the administrator. */
		void sekey_printlog(std::string& msg);

		int sekey_user_init(std::string& user_id, std::string& username, std::string& sn);

		/** @brief Iterate over the recovery table of SEkey processing all the recovery requests. Available only for the administrator.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details Generate the recovery file for all users who need it. Return error if the generation
		 * of the recovery file is not completed correctly for all users inside the recovery table.
		 * The policy of SEkey APIs is to prevent the execution of the API if the recovery table is not
		 * empty. May throw exceptions. This function will call the sekey_write_recovery(). */
		int sekey_recovery();

		/** @brief Generate the recovery file for a specific user. Available only for the administrator.
		 * @param [in] user_id The ID of the user who needs a SEkey data recovery.
		 * @param [in] serial_number The serial number of the SEcube of the user.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This function generates the recovery file for a user, given its ID and serial number. If the couple (user, serial number) is not found, this function will simply generate
		 * an update encrypted with a universal key (known as wildcard, known by all present and past users of SEkey) in order to force the user to delete its SEkey content (if any).
		 * If the user is found in SEkey, the recovery file is generated inserting all the informations that the user needs to rebuild its SEkey database from scratch. If the recovery file is generated
		 * correctly, the user is removed from the list of users (Recovery table) who need recovery, otherwise the user stays in the list. */
		int sekey_write_recovery(std::string& user_id, std::string& serial_number);

		/** @brief Remove a user from list of users who need a complete recovery of the SEkey database. Available only for the administrator.
		 * @param [in] user_id The ID of the user to be deleted from the recovery list.
		 * @param [in] sn The serial number of the user's SEcube.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details Used internally by SEkey, should not be used by higher levels. */
		int reset_user_recovery(std::string& user_id, std::string& sn);

		/** @brief Function to write in the update file of a user the request to delete entirely another user from SEkey.
		 * @param [in] user_id The ID of the user who needs the update.
		 * @param [in] uid The ID of the user to be deleted.
		 * @param [in] erase A flag to request also the deletion of the user from the recovery list in case the update generation succeeds.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * In case user_id and uid are the same, the overloaded version is used. Used internally by SEkey, do not use otherwise. */
		void req_delete_user(std::string& user_id, std::string& uid, bool erase);

		/** @brief Function to write in the update file of a user the request to delete the very same user from SEkey.
		 * @param [in] user_id The ID of the user who needs the update (who is also the user to be deleted).
		 * @param [in] algo The algorithm to be used to write the update file.
		 * @param [in] key_id The key to be used to encrypt the update file.
		 * @param [in] sn The serial number of the user to be deleted.
		 * @param [in] erase A flag to request also the deletion of the user from the recovery list in case the update generation succeeds.
		 * @param [in] mode The mode to open the update file to be written (i.e. init, recovery, normal).
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details Used internally by SEkey (admin mode only) in order to implement the complex update logic. Do not use otherwise. */
		void req_delete_user(std::string& user_id, uint32_t algo, uint32_t key_id, std::string& sn, bool erase, int mode);

		/** @brief Function to write in the update file of a user the request to delete a group from SEkey.
		 * @param [in] user_id The ID of the user who needs the update.
		 * @param [in] gid The ID of the group to be deleted.
		 * @param [in] erase A flag to request also the deletion of the user from the recovery list in case the update generation succeeds.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details Used internally by SEkey (admin mode only) in order to implement the complex update logic. Do not use otherwise. */
		void req_delete_group(std::string& user_id, std::string& gid, bool erase);

		/** @brief Function to write in the update file of a user the request to delete a user from a group. Available only for the administrator.
		 * @param [in] user_id The ID of the user who needs the update.
		 * @param [in] uid The ID of the user to be deleted (may or may not be the same as user_id).
		 * @param [in] group_id The ID of the group from which the user must be deleted.
		 * @param [in] erase A flag to request also the deletion of the user from the recovery list in case the update generation succeeds (see reset_user_recovery()).
		 * @details Used internally by SEkey, do not use otherwise. No throw. */
		void req_delete_user_from_group(std::string& user_id, std::string& uid, std::string& group_id, bool erase);

		/** @brief Function executed only when SEkey is running in user mode. This performs the actions requested by req_delete_user_from_group().
		 * @param [in] buffer The buffer to be deserialized with all the info about the operations to be done.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details Used by SEkey to apply a specific update request, do not use otherwise. The function will delete the requested user from the specified group. */
		int usr_delete_user_from_group(char *buffer);

		/** @brief Function executed only when SEkey is running in user mode. This performs the actions requested by req_delete_user().
		 * @param [in] buffer The buffer to be deserialized with all the info about the operations to be done.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This function will delete the specified user from SEkey, modifying the user's SEkey database. If the user to be deleted is the same user executing
		 * the update, then all SEkey informations (both in the database and in the SEcube flash) will be deleted. Used internally by SEkey, do not use otherwise. */
		int usr_delete_user(char *buffer);

		/** @brief Function to write in a secure way a key of SEkey to the update file of a user. Available only for the administrator.
		 * @param [in] user_id The ID of the user who needs the update.
		 * @param [in] kid The ID of the key to be sent.
		 * @param [in] key_len The length of the key to be sent.
		 * @param [in] erase A flag to request also the deletion of the user from the recovery list (see reset_user_recovery()) in case the update generation succeeds.
		 * @details Used internally by SEkey, do not use otherwise. May throw exceptions. */
		void send_key_update(std::string& user_id, uint32_t kid, uint32_t key_len, bool erase);

		/** @brief Function executed only when SEkey is running in user mode. This performs the actions requested by req_delete_group().
		 * @param [in] buffer The buffer to be deserialized with all the info about the operations to be done.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This function is used internally by SEkey in order to manage the complex update logic. It should not be used directly by
		 * the higher level application. The function will delete the specified group from SEkey, modifying the user's SEkey database. */
		int usr_delete_group(char *buffer);

		/** @brief Function executed only when SEkey is running in user mode. Execute operations requested by send_key_update().
		 * @param [in] buffer The buffer to be deserialized with all the info about the operations to be done.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * This function is used internally by SEkey in order to manage the complex update logic. It should not be used directly by
		 * the higher level application. The function will retrieve a key written in the update file and store it in the SEcube flash
		 * memory; the key will not be visible as plaintext in the host computer (neither on the disk or in RAM) because it is encrypted
		 * end-to-end from the admin SEcube to the user SEcube. */
		int usr_store_key(char *buffer);

		/** @brief Send to a new SEkey user the update containing basic info to initialize his SEkey database.
		 * @param [in] user_id The ID of the user who needs the init update.
		 * @param [in] query The SQL query to be run in order to update the SEkey user database.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * Used by SEkey only in admin mode. Do not use otherwise. */
		int send_user_init_update(std::string& user_id, std::string& query);

		/** @brief Retrieves the ID of all the keys stored inside the flash of the SEcube. If the ID is not found
		 * inside SEkey and the ID is not reserved (meaning that the key is a normal key of SEkey) then that
		 * key should not be in the flash and it is deleted. This is a simple garbage collector that will keep
		 * the flash of the SEcube clean from everything that should not be there.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise. */
		void se3_flash_maintenance_routine();

		/** @brief Function executed only when SEkey is running in user mode. This function will execute a SQL query written in the update file of the user.
		 * @param [in] buffer The buffer to be deserialized with all the info about the operations to be done.
		 * @param [in] bufsize The length of the SQL query to be executed.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * Used internally by SEkey, do not use otherwise. */
		int usr_sql_exec(char *buffer, uint32_t bufsize);

		/** @brief Read an update file (of any type) and process its content.
		 * @param [in] filepath The path of the file to process.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details Used internally by SEkey, should not be used by higher levels. */
		int execute_update(std::string& filepath);

		/** @brief Write a SQL query to the update file of a specific user.
		 * @param [in] user_id ID of the user who is the destination of the update file.
		 * @param [in] query SQL query to be written to the file and executed by the user.
		 * @param [in] erase If true delete user from recovery table upon success, if false do not.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise. */
		void send_sql_update(std::string& user_id, std::string& query, bool erase);

		/** @brief Open an update file of a specific user.
		 * @param [in] updatefile The SEfile object related to the file to open.
		 * @param [in] sn The serial number of the user.
		 * @param [in] overwrite Flag to force the override of the current file (if any).
		 * @param [in] create Flag to enable the creation of the file, if not present.
		 * @param [in] mode Specifies if init file, recovery file or normal file.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This is basically a wrapper for the SEfile::secure_open() function of SEfile. The parameters
		 * are simply used to take decisions before calling the SEfile::secure_open() on the updatefile parameter. */
		int open_update_file(SEfile& updatefile, std::string& sn, bool overwrite, bool create, int mode);

		/** @brief Check if a key status transition is allowed or not.
		 * @param [in] current_status The current key status.
		 * @param [in] new_status The new key status.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details A key status transition is allowed only in very specific circumstances. For example a key can change
		 * its status to active only if its previous status was preactive or suspended. On the other hand a key status
		 * can always be changed to destroyed but a key whose status is destroyed can't be activated anymore. Therefore this
		 * function checks for these (and more) constraints. */
		int check_key_transition_validity(se_key_status current_status, se_key_status new_status);

		/** @brief Process updates for the current user.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This function is executed only by the user application and is used to check if there are updates
		 * for the SEkey database of the user. There are 3 types of updates, following a specific hierarchy: init,
		 * recovery and normal. The init update file is generated only once when the SEcube of the user is initialized.
		 * The recovery update file is generated every time that the admin issues a recovery for the user or every time
		 * that the user asks specifically to the admin to do a recovery. The normal update file is the file generated
		 * by the administrator of SEkey when everything works normally. These three updates must be executed strictly
		 * in order, i.e. the user cannot execute the normal update file if there is a recovery update file pending.
		 * This is fundamental to implement correctly the database update mechanism and to grant that the SEkey
		 * database of the user is consistent in time. This function may throw; it is called automatically by the other
		 * API called sekey_update_userdata(). */
		int process_update_file();

		/** @brief Wrapper around req_delete_user() to invoke the function for each user in the users vector passed as argument.
		 * @param [in] users The array of user IDs on which the req_delete_user() function must be invoked.
		 * @param [in] user_id The ID of the user to be deleted.
		 * @param [in] erase A flag to request also the deletion of the user from the recovery list in case the update generation succeeds.
		 * @details Used internally by SEkey (admin mode only) in order to implement the complex update logic. Do not use otherwise. */
		void delete_user_iterator(std::vector<std::string>& users, std::string& user_id, bool erase);

		/** @brief Wrapper around req_delete_group() to invoke the function for each user in the users vector passed as argument.
		 * @param [in] users The array of user IDs on which the req_delete_group() function must be invoked.
		 * @param [in] group_id The ID of the group to be deleted.
		 * @param [in] erase A flag to request also the deletion of the user from the recovery list in case the update generation succeeds.
		 * @details Used internally by SEkey (admin mode only) in order to implement the complex update logic. Do not use otherwise. */
		void delete_group_iterator(std::vector<std::string>& users, std::string& group_id, bool erase);

		/** @brief Wrapper to execute send_sql_update() for all users inside a list.
		 * @param [in] users The users involved by this update.
		 * @param [in] query The query to be written to the update files.
		 * @param [in] erase Tells to send_sql_update() if the user should also be removed from recovery upon success.
		 * @details This function is a wrapper that allows to execute the send_sql_update() function for all users listed inside
		 * the array of user IDs passed to the wrapper itself. This means that the query will be written to several update files,
		 * one for each user included in the array. */
		void sql_update_iterator(std::vector<std::string>& users, std::string& query, bool erase);

		/** @brief Wrapper around req_delete_user_from_group() to invoke the function for each user in the users vector passed as argument.
		 * @param [in] users The array of user IDs on which the req_delete_user_from_group() function must be invoked.
		 * @param [in] user_id The ID of the user to be deleted.
		 * @param [in] group_id The ID of the group from which the user must be deleted.
		 * @param [in] erase A flag to request also the deletion of the user from the recovery list in case the update generation succeeds.
		 * @details Used internally by SEkey (admin mode only) in order to implement the complex update logic. Do not use otherwise.*/
		void delete_user_from_group_iterator(std::vector<std::string>& users, std::string& user_id, std::string& group_id, bool erase);

		/** @brief Wrapper around send_key_update() to invoke the function for each user in the users vector passed as argument.
		 * @param [in] users The array of user IDs on which the send_key_update() function must be invoked.
		 * @param [in] kid The ID of the key to be sent.
		 * @param [in] key_len The length of the key to be sent.
		 * @param [in] erase A flag to request also the deletion of the user from the recovery list in case the update generation succeeds.
		 * @details Used internally by SEkey (admin mode only) in order to implement the complex update logic. Do not use otherwise.*/
		void key_update_iterator(std::vector<std::string>& users, uint32_t kid, uint32_t key_len, bool erase);

		/** @brief Rollback a SQLite transaction.
		 * @return SEKEY_UNCHANGED in case of rollback success, SEKEY_RESTART in case of error.
		 * @details This function attempts to rollback the database following an unexpected error. In case of failure of
		 * the first rollback query, the function will wait a limited amount of time (configured to 1 second by default)
		 * before attempting the rollback again. If the rollback fails multiple times (set to 3 by default) the function
		 * will return SEKEY_RESTART in order to signal to the caller that the application should be restarted. When the
		 * application will be relaunched, the rollback will be issued automatically by SQLite thanks to the journaling
		 * file left on disk. In case of correct rollback the function will return SEKEY_UNCHANGED in order to inform the
		 * caller that the database is not changed with respect to the last successfull operation. Notice that a rollback
		 * may leave unnecessary data in the flash of the SEcube but these data will be deleted by the garbage collector.
		 * This function does not throw any exception. */
		int rollback_transaction();

		/** @brief Commit a SQLite transaction.
		 * @return SEKEY_OK, SEKEY_UNCHANGED, SEKEY_RESTART in case of exception during commit or failed rollback.
		 * @details Try (up to 3 times) to commit a transaction. If commit is successful no problem, otherwise try
		 * to revert back the changes. If even rollback is not feasible, ask to restart the application. This function
		 * never throws exceptions. */
		int commit_transaction();

		/** @brief Add one or more users of SEkey to the list of users who need recovery.
		 * @param [in] users The list (even with only one element) of users to be added to the recovery.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details Used only in admin mode. Do not use otherwise. */
		int fill_recovery(std::vector<std::string>& users);

		/** @brief Executes a SQLite query (with one parameter to bind) and stores the results in a vector of strings.
		 * @param [in] query The parameter to bind inside the query.
		 * @param [in] query The query to be executed.
		 * @param [out] The vector of strings that will be filled by this function.
		 * @return 0 upon success, -1 in case of errors. May throw.
		 * @details This is a handy wrapper that executes a query and stores the results in an array of strings. Notice that
		 * it is intended to be used only to fill a container with strings that have the same meaning. Therefore this function
		 * is normally used with a query that selects only one attribute for each row that matches the query itself. For example
		 * it is used to fill a vector of user IDs with the ID of each user of SEkey who belongs to a specific group. Internally used by SEkey, do not use otherwise. */
		int sql_fill_vector(std::string *bind, std::string& query, std::vector<std::string> *container);

		/** @brief Wrapper around sqlite3_column_int64() to retrieve the corresponding 32 bit unsigned value.
		 * @return The value from the database, throws an exception if the value can't be stored on 32 bits.
		 * @details This is necessary because all the integers stored in the database are set to 64 bit signed value.
		 * This is done in order to have all integers of the same type inside the db, instead of letting
		 * SQLite to choose automatically what kind of integer to use (16,32,64 bits). Each function internally
		 * will manage these values according to the real needs of SEkey, for example a key ID is always 4 bytes
		 * while the key algorithm is stored on 2 bytes. */
		uint32_t get_u32(sqlite3_stmt *stmt, int index);

		/** @brief Check if the algorithm is valid. In order to be valid, the algorithm should be included in L1Algorithms::Algorithm.
		 * Check L1_enumerations.h for more info.
		 * @param [in] algorithm The algorithm to be checked.
		 * @return True if valid, false otherwise. */
		bool algovalid(uint32_t algorithm);

		/** @brief Check if a user is already stored in the SEkey KMS.
		 * @param [in] user_id The id of the user to search.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise. */
		int is_user_present(std::string& user_id);

		/** @brief Same as is_user_present(), simply written for groups. May throw. */
		int is_group_present(std::string& group_id);

		/** @brief Same as is_user_present(), simply written for keys. May throw. */
		int is_key_present(std::string& key_id);

		/** @brief Delete a file encrypted with SEfile. This embeds plaintext filepath translation to encrypted filepath used by SEfile.
		 * @param [in] The SEfile object related to the file, if any. Used to close the file before deleting it.
		 * @param [in] The path of the file.
		 * @return True if the file is remove successfully, false otherwise. May throw.*/
		bool deletefile(SEfile *fileptr, std::string& filepath);

		/** @brief Check if a SEfile file exists. Automatically translate the plaintext filename to the encrypted filename used by SEfile.
		 * @param [in] The name of the file expressed as plaintext.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise. */
		int file_exists(std::string& filename);

		/** @brief Generate a 32 byte serial number for a SEcube device.
		 * @param [out] sn The 32 byte long buffer which will contain the serial number.
		 * @return 0 upon success, -1 otherwise. No throw.
		 * Each SEcube is initialized with a unique serial number of 32 bytes. The serial number
		 * is generated using random numbers in the interval [0-9], then the current epoch time
		 * is copied in the first positions of the buffer that holds the serial number (i.e. 123 becomes
		 * "123" using the first 3 positions in the serial number). Since the time is always moving forward,
		 * there will never be two equal serial numbers because epoch will always be different. */
		int generate_serial_number(std::array<uint8_t, L0Communication::Size::SERIAL>& sn);

		/** @brief Check if SEkey in user mode is updated to latest version. If not block every operation. */
		bool user_allowed();

		/** @brief Check if algo1 is stronger than algo2. Return 1 if stronger, return -1 if weaker, 0 if equal. */
		int algocmp(uint32_t algo1, uint32_t algo2);

		/** @brief Wrapper around the sqlite3_expanded_sql() function from SQLite.
		 * @param [in] stmt The statement to retrieve.
		 * @param [out] s The string that will contain the statement.
		 * @return Returns SEKEY_OK upon success, a value from \ref sekey_error otherwise.
		 * @details This is simply a wrapper around the sqlite3_expanded_sql() used to
		 * retrieve the SQL query after the prepared statement and binding procedure.
		 * A wrapper is required in order to ensure that the memory allocated by
		 * the original SQLite API is always freed. */
		int sqlite3_expanded_sql_wrapper(sqlite3_stmt *stmt, std::string& s);

		/** @brief Safe wrapper around the sqlite3_column_text() API of SQLite. */
		std::string sqlite3_column_text_wrapper(sqlite3_stmt *stmt, int col);

		/** @brief Checks if an ID matches the corresponding regular expression.
		 *  @param [in] in String containing the ID to check.
		 *  @param [in] sel Integer identifying which kind of ID (user, group, or key).
		 *  @return True if it matches, false otherwise. */
		bool check_input(std::string& in, uint8_t sel);
		int get_microsd_path(L0& l0, std::string& microsd);
	///@}
/** @}*/

#endif
