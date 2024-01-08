#include "routers/access_control_router.hpp"

define::AccessControl self::router::AccessControlRouter::getAccessControl(std::string_view autheHeader)
{
	define::AccessControl accessControl;

	std::string bearer{ "Bearer " }, token{ "" };

	size_t pos = autheHeader.find(bearer);
	if (pos != std::string::npos) {
		token = autheHeader.substr(pos + bearer.length());
	}

	bool is_exist{ false };

	global::db::local << "select count(*) from access_control where token = ?;" << token >> is_exist;

	if (not is_exist) throw self::HTTPException("this token does not exist", 400);

	global::db::local << "select sid,account,hits,token,authority from access_control where token = ?;"
		<< token
		>> [&](unsigned int sid, std::string  account, unsigned int hits, std::string token, unsigned char authority) {
		accessControl.hits = hits;
		accessControl.authority = authority;
		accessControl.sid = sid;
		accessControl.token = token;
		accessControl.account = account;
	};

	global::db::local << "UPDATE access_control SET hits = (hits + 1) WHERE sid = ?;"
		<< accessControl.sid;

	return accessControl;
}

self::router::AccessControlRouter::AccessControlRouter(std::shared_ptr<CrowApplication> app) : m_app{ app } {
	LogSystem::logInfo("权限操作相关路由初始化完毕");
}

void self::router::AccessControlRouter::router() {
	CROW_ROUTE((*m_app), "/manage/userlist").methods(crow::HTTPMethod::Get)([&](const crow::request req) {
		LogSystem::logInfo("用户列表获取");
		return self::HandleResponseBody([&] {
			auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
			if (accessControl.authority < 5)
			{
				LogSystem::logInfo(std::format("用户列表获取失败 -- {}({})权限不足", accessControl.account, accessControl.sid));
				throw self::HTTPException("", 401);
			}

			json result;

			global::db::local << "select sid,account,hits,token,authority from access_control;"
				>> [&](unsigned int sid, std::string  account, unsigned int hits, std::string token, unsigned char authority) {
				result[account] = {
					{"sid", sid},
					{"hits", hits},
					{"token", token},
					{"authority", authority}
				};
			};

			LogSystem::logInfo("用户列表获取成功");

			return result.dump();
			});
		});
	CROW_ROUTE((*m_app), "/manage/add").methods(crow::HTTPMethod::Post)([&](const crow::request req) {
		return self::HandleResponseBody([&] {
			LogSystem::logInfo("添加授权用户");

			auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
			if (accessControl.authority < 5)
			{
				LogSystem::logInfo(std::format("添加授权用户失败 -- {}({})权限不足", accessControl.account, accessControl.sid));
				throw self::HTTPException("", 401);
			}

			json result;


			json data{ json::parse(req.body) };
			std::exchange(data, data[0]);

			global::db::local << "insert into access_control (account,token,authority) values (?,?,?);"
				<< data.at("account").get<std::string>()
				<< self::common::utils::generateRandom()
				<< data.at("authority").get<uint8_t>();

			LogSystem::logInfo("添加授权用户成功");
			return "ok";
			});
		});
	CROW_ROUTE((*m_app), "/manage/permissions/<int>/<int>").methods(crow::HTTPMethod::Put)([&](const crow::request req, int sid, int auth) {
		return self::HandleResponseBody([&] {
			LogSystem::logInfo("修改权限");
			auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
			if (accessControl.authority < 5)
			{
				LogSystem::logInfo(std::format("修改权限失败 -- {}({})权限不足", accessControl.account, accessControl.sid));
				throw self::HTTPException("", 401);
			}

			global::db::local << "UPDATE access_control SET authority = ? WHERE sid = ?"
				<< auth
				<< sid;

			LogSystem::logInfo("修改权限成功");

			return "ok";
			});
		});
}
