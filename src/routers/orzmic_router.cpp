#include "routers/orzmic_router.hpp"
#include "common/prevent_inject.hpp"
#include <cpprest/http_client.h>
#include <cpprest/json.h>

void self::router::OrzmicRouter::replaceStr(std::string& str) {
	self::common::utils::replaceStrAll(str, "\"", "\"\"");
}

void self::router::OrzmicRouter::updateSearch(int32_t music_id) {
	json music_info;
	global::db::orzmic << "select music_id,file_name,title from songs where music_id = ?;" << music_id >> [&]
	(int32_t music_id, std::string file_name, std::string title) {
		music_info["MusicId"] = music_id;

		music_info["FileName"] = file_name;
		music_info["Title"] = title;
		global::db::orzmic << "select id,alias from alias where music_id = ?" << music_id >> [&]
		(int32_t id, std::string alias) {
			json alias_data;
			music_info["Aliases"].emplace_back(json{
				{"AliasId", id},
				{"Alias", alias},
				});
		};
	};

	web::http::client::http_client client(U(global::search::meiliURL));
	// 创建第一个HTTP请求, 添加匹配索引
	web::http::http_request request_add_index(web::http::methods::POST);
	request_add_index.set_request_uri("/indexes/"s + global::search::orzmicSearchNamespace + "/documents?primaryKey=MusicId"s);
	request_add_index.headers().add("Content-Type", "application/json");
	request_add_index.headers().add("Authorization", "Bearer "s + global::search::meiliAuth);
	request_add_index.set_body(web::json::value::parse(music_info.dump()));

	auto response = client.request(request_add_index).get();

	if (response.status_code() >= 300 or response.status_code() < 200) {
		auto error = response.extract_string().get();
		throw self::HTTPException(error, 500);
	}
}

define::AccessControl self::router::OrzmicRouter::getAccessControl(std::string_view autheHeader)
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

self::router::OrzmicRouter::OrzmicRouter(std::shared_ptr<CrowApplication> app) : m_app{ app } {
	LogSystem::logInfo("Orzmic路由初始化完毕");
}

void self::router::OrzmicRouter::router() {
	CROW_ROUTE((*m_app), "/api/orzmic/getMusic").methods(crow::HTTPMethod::Post)([&](const crow::request& req) {
		return self::HandleResponseBody([&] {
			LogSystem::logInfo("[Orzmic]曲目资源获取");
			auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
			if (accessControl.authority < 1) {
				LogSystem::logInfo(std::format("[Orzmic]曲目资源获取失败 -- {}({})权限不足", accessControl.account, accessControl.sid));
				throw self::HTTPException("", 401);
			}

			std::string key{};
			const std::array<std::string, 3> where_field_names{ "music_id", "file_name", "title" }; // field_index: 0, 1, 2
			int mode{ 0 }; // 0 完全匹配, 1 模糊匹配

			json result;

			json data{ json::parse(req.body) };
			std::exchange(data, data[0]);

			if (data.count("key")) {
				key = data.at("key").get<std::string>();
				if (key.empty())throw self::HTTPException("'key' is empty", 400);
			}
			else throw self::HTTPException("request body missing 'key'", 400);

			if (data.count("mode")) {
				mode = data.at("mode").get<int>();
			}

			std::string front_sql{ "select * from songs " };

			if (mode == 0) {
				/* 通过MusicID/FileName/Title匹配资源 */
				front_sql += "where ";
				bool is_nocase{ false }; // 是否有大小写匹配问题
				std::size_t field_index{ 0 }; // 单个查询的检索模式
				if (data.count("is_nocase")) { // 是否对大小写操作
					is_nocase = data.at("is_nocase").get<bool>();
				}
				if (data.count("field_index")) { // 检索方式
					field_index = data.at("field_index").get<std::size_t>();
				}

				if (field_index > (where_field_names.size() - 1)) throw self::HTTPException("index out of bounds", 400);

				auto field_name{ where_field_names.at(field_index) };

				front_sql += field_name;

				if (field_index == 0) {
					if (not self::CheckParameterStr(key))throw self::HTTPException("SQL injection may exist", 403);
					front_sql += " = "s + key;
				} elif(field_index == 1) {
					if (not self::CheckParameterStr(key))throw self::HTTPException("SQL injection may exist", 403);
					front_sql += " = \""s + key + "\"";
				} elif(field_index == 2) {
					replaceStr(key);
					front_sql += " = \""s + key + "\"";

					if (is_nocase) {
						front_sql += " COLLATE NOCASE";
					}
				}
				else throw self::HTTPException("", 500);
			}elif(mode == 1) {
				/* 获取标题匹配资源 */
				front_sql += "where title like \"%" + key + "%\"";
			}elif(mode == 2) { /* 获取全部资源 */ }
			else {
				throw self::HTTPException("this mode does not exist", 400);
			}

			global::db::orzmic << front_sql
				>> [&](__ORZMIC_SQL_ARGS) {
				json music{
					{"MusicID", music_id},
					{"FileName", file_name},
					{"Title", title},
					{"Lockhint", lock_hint},
					{"InitialUnlock", initial_unlock},
					{"Watermark", watermark},
					{"Artist", artist},
					{"CoverPainter", cover_painter},
					{"BPMRange", bpm},
					{"AudioPreviewFrom", audio_preview_from},
					{"AudioPreviewTo", audio_preview_to}
				};

				// chart_designer_easy difficulty_easy note_count_easy rating_easy
				// chart_designer_normal difficulty_normal note_count_normal rating_normal
				// chart_designer_hard difficulty_hard note_count_hard rating_hard
				// chart_designer_special difficulty_special note_count_special rating_special

				constexpr std::size_t max_level{ 3 };

				for (std::size_t index{ 0 }; index <= max_level; ++index) {
					music["Difficulties"][index] = json{
							{"ChartDesigner", nullptr},
							{"Difficulty", nullptr},
							{"NoteCount", nullptr},
							{"Rating", nullptr},
					};

					define::OrzmicLevel level;
					if (index == 0) {
						if (chart_designer_easy) music["Difficulties"][index]["ChartDesigner"] = *chart_designer_easy;
						if (difficulty_easy) music["Difficulties"][index]["Difficulty"] = *difficulty_easy;
						if (note_count_easy) music["Difficulties"][index]["NoteCount"] = *note_count_easy;
						if (rating_easy) music["Difficulties"][index]["Rating"] = *rating_easy;
					} elif(index == 1) {
						if (chart_designer_normal) music["Difficulties"][index]["ChartDesigner"] = *chart_designer_normal;
						if (difficulty_normal) music["Difficulties"][index]["Difficulty"] = *difficulty_normal;
						if (note_count_normal) music["Difficulties"][index]["NoteCount"] = *note_count_normal;
						if (rating_normal) music["Difficulties"][index]["Rating"] = *rating_normal;
					} elif(index == 2) {
						if (chart_designer_hard) music["Difficulties"][index]["ChartDesigner"] = *chart_designer_hard;
						if (difficulty_hard) music["Difficulties"][index]["Difficulty"] = *difficulty_hard;
						if (note_count_hard) music["Difficulties"][index]["NoteCount"] = *note_count_hard;
						if (rating_hard) music["Difficulties"][index]["Rating"] = *rating_hard;
					} elif(index == 3) {
						if (chart_designer_special) music["Difficulties"][index]["ChartDesigner"] = *chart_designer_special;
						if (difficulty_special) music["Difficulties"][index]["Difficulty"] = *difficulty_special;
						if (note_count_special) music["Difficulties"][index]["NoteCount"] = *note_count_special;
						if (rating_special) music["Difficulties"][index]["Rating"] = *rating_special;
					}
				}

				music["Additional"] = json::parse(extra_content);

				result.push_back(music);
			};

			LogSystem::logInfo("[Orzmic]曲目资源完成");
			return result.dump();
			});
		});

	CROW_ROUTE((*m_app), "/api/orzmic/addAlias").methods(crow::HTTPMethod::Post)([&](const crow::request& req) {
		return self::HandleResponseBody([&] {
			LogSystem::logInfo("[Orzmic]添加别名");
			auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
			if (accessControl.authority < 3) {
				LogSystem::logInfo(std::format("[Orzmic]添加别名失败 -- {}({})权限不足", accessControl.account, accessControl.sid));
				throw self::HTTPException("", 401);
			}

			json data{ json::parse(req.body) };
			std::exchange(data, data[0]);

			std::int32_t relatedId = data.at("relatedId").get<std::int32_t>();
			std::string alias = data.at("alias").get<std::string>();

			bool is_existe{ false };
			global::db::orzmic << "select count(music_id) from songs where music_id = ?;" << relatedId >> is_existe;

			if (not is_existe) throw self::HTTPException("music id does not exist", 404);
			global::db::orzmic << "insert into alias (alias,music_id) values (?,?);"
				<< alias
				<< relatedId;

			if (global::search::isOpen) this->updateSearch(relatedId);

			LogSystem::logInfo("[Orzmic]添加别名完成");
			return "ok";
			});
		});

	CROW_ROUTE((*m_app), "/api/orzmic/delAlias").methods(crow::HTTPMethod::Post)([&](const crow::request& req) {
		return self::HandleResponseBody([&] {
			LogSystem::logInfo("[Orzmic]删除别名");
			auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
			if (accessControl.authority < 3) {
				LogSystem::logInfo(std::format("[Orzmic]删除别名失败 -- {}({})权限不足", accessControl.account, accessControl.sid));
				throw self::HTTPException("", 401);
			}

			json data{ json::parse(req.body) };
			std::exchange(data, data[0]);

			std::int32_t alias_id = data.at("aliasId").get<std::int32_t>();

			bool is_existe{ false };
			std::int32_t relatedId{ -1 };

			global::db::orzmic << "select count(id),music_id from alias where id = ?;" << alias_id >> [&]
			(bool count_id, int32_t music_id) {
				is_existe = count_id;
				relatedId = music_id;
			};

			if (not is_existe) throw self::HTTPException("music id does not exist", 404);

			global::db::orzmic << "delete from alias where id = ?;" << alias_id;

			if (global::search::isOpen) this->updateSearch(relatedId);

			LogSystem::logInfo("[Orzmic]删除别名完成");
			return "ok";
			});
		});

	CROW_ROUTE((*m_app), "/api/orzmic/getAlias").methods(crow::HTTPMethod::Post)([&](const crow::request& req) {
		return self::HandleResponseBody([&] {
			LogSystem::logInfo("[Orzmic]获取别名");
			auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
			if (accessControl.authority < 3) {
				LogSystem::logInfo(std::format("[Orzmic]获取别名失败 -- {}({})权限不足", accessControl.account, accessControl.sid));
				throw self::HTTPException("", 401);
			}
			json result;
			json data{ json::parse(req.body) };
			std::exchange(data, data[0]);

			std::string key{""};
			int mode{ 0 };
			bool is_nocase{ false };

			if (data.count("key")) {
				key = data.at("key").get<std::string>();
			}
			else throw self::HTTPException("request body missing 'key'", 400);
			if (data.count("mode")) mode = data.at("mode").get<int>();
			if (data.count("is_nocase")) is_nocase = data.at("is_nocase").get<bool>();

			if (mode == 0) {
				if (key.empty())throw self::HTTPException("'key' is empty", 400);
				int32_t music_id{};
				this->replaceStr(key);
				std::string sql{ "select music_id from alias where alias = \"" + key + "\" " };

				if (is_nocase) {
					sql += "COLLATE NOCASE";
				}

				global::db::orzmic << sql >> music_id;

				result["MusicId"] = music_id;
			}elif(mode == 1) {
				if (not self::CheckParameterStr(key))throw self::HTTPException("SQL injection may exist", 403);

				if (key.empty()) {
					global::db::orzmic << "select music_id from songs;" >> [&](int32_t music_id) {
						bool is_exist{ false };
						global::db::orzmic << "select count(*) from alias where music_id = ?;"
							<< music_id >> is_exist;
						if (is_exist){
							json res;
							res["MusicId"] = music_id;
							global::db::orzmic << "select id, alias from alias where music_id = ?;"
								<< music_id
								>> [&](std::int32_t id, std::string alias) {
								res["Content"].emplace_back(json{
									{"AliasId", id},
									// {"MusicId", music_id},
									{"Alias", alias},
									});
							};
							result["Contents"].push_back(res);
						}
					};
				} else {
					std::int32_t music_id = std::stoi(key);
					global::db::orzmic << "select id, alias from alias where music_id = ?;"
						<< music_id
						>> [&](std::int32_t id, std::string alias) {
						json data{
							{"AliasId", id},
							// {"MusicId", music_id},
							{"Alias", alias},
						};
						result["MusicId"] = music_id;
						result["Content"].emplace_back(data);
					};
				}
			} else {
				throw self::HTTPException("this mode does not exist", 400);
			}

			if (result.is_null()) throw self::HTTPException("", 404);
			LogSystem::logInfo("[Orzmic]获取别名完成");
			return result.dump();
			});
		});

	if (global::search::isOpen) {
		CROW_ROUTE((*m_app), "/api/orzmic/syncAlias").methods(crow::HTTPMethod::Post)([&](const crow::request& req) {
			return self::HandleResponseBody([&] {
				LogSystem::logInfo("[Orzmic]同步别名匹配");
				auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
				if (accessControl.authority < 4) {
					LogSystem::logInfo(std::format("[Orzmic]同步别名匹配失败 -- {}({})权限不足", accessControl.account, accessControl.sid));
					throw self::HTTPException("", 401);
				}

				json body;

				global::db::orzmic << "select music_id,file_name,title from songs" >>
					[&](int32_t music_id, std::string file_name, std::string title) {
					json music_info;

					music_info["MusicId"] = music_id;

					music_info["FileName"] = file_name;
					music_info["Title"] = title;
					global::db::orzmic << "select id,alias from alias where music_id = ?" << music_id >> [&]
					(int32_t id, std::string alias) {
						json alias_data;
						music_info["Aliases"].emplace_back(json{
							{"AliasId", id},
							{"Alias", alias},
							});
					};

					body.emplace_back(music_info);
				};

				web::http::client::http_client client(U(global::search::meiliURL));
				// 创建第一个HTTP请求, 添加匹配索引
				web::http::http_request request_add_index(web::http::methods::POST);
				request_add_index.set_request_uri("/indexes/"s + global::search::orzmicSearchNamespace + "/documents?primaryKey=MusicId"s);
				request_add_index.headers().add("Content-Type", "application/json");
				request_add_index.headers().add("Authorization", "Bearer "s + global::search::meiliAuth);
				request_add_index.set_body(web::json::value::parse(body.dump()));

				// 创建第二个HTTP请求设置只对特定数据索引
				web::http::http_request request_searchable_attributes(web::http::methods::PUT);
				request_searchable_attributes.set_request_uri("/indexes/"s + global::search::orzmicSearchNamespace + "/settings/searchable-attributes"s);
				request_searchable_attributes.headers().add("Authorization", "Bearer "s + global::search::meiliAuth);
				request_searchable_attributes.set_body(web::json::value::parse("[\"Aliases.Alias\",\"Title\"]"));


				// 发送多个HTTP请求并获取响应(异步操作)
				std::vector<pplx::task<web::http::http_response>> tasks;
				tasks.push_back(client.request(request_add_index));
				tasks.push_back(client.request(request_searchable_attributes));

				pplx::when_all(tasks.begin(), tasks.end()).then([](std::vector<web::http::http_response> responses) {
					// 处理响应
					for (const auto& resp : responses) {
						if (resp.status_code() >= 300 or resp.status_code() < 200) {
							auto error = resp.extract_string().get();
							throw self::HTTPException(error, 500);
						}
					}
					}).wait();

					LogSystem::logInfo("[Orzmic]同步别名完成");
					return "ok";
				});
			});

		CROW_ROUTE((*m_app), "/api/orzmic/matchAlias").methods(crow::HTTPMethod::Post)([&](const crow::request& req) {
			return self::HandleResponseBody([&] {
				LogSystem::logInfo("[Orzmic]曲目匹配");
				auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
				if (accessControl.authority < 1) {
					LogSystem::logInfo(std::format("[Orzmic]搜索曲目失败 -- {}({})权限不足", accessControl.account, accessControl.sid));
					throw self::HTTPException("", 401);
				}

				json data{ json::parse(req.body) };
				std::exchange(data, data[0]);

				std::string query{};
				std::uint16_t limit{ 20 }, offset{ 0 }, hitsPerPage{ 20 }, page{ 1 };
				bool showRankingScore{ true };

				if (data.count("query")) query = data.at("query").get<std::string>();
				if (data.count("limit")) limit = data.at("limit").get<uint16_t>();
				if (data.count("offset")) offset = data.at("offset").get<uint16_t>();
				if (data.count("hitsPerPage")) hitsPerPage = data.at("hitsPerPage").get<uint16_t>();
				if (data.count("page")) page = data.at("page").get<uint16_t>();
				if (data.count("showRankingScore")) showRankingScore = data.at("showRankingScore").get<bool>();
				
				json body{
					{"q", query},
					{"limit", limit},
					{"offset", offset},
					{"hitsPerPage", hitsPerPage},
					{"page", page},
					{"showRankingScore", showRankingScore},
				}, resp, result;

				web::http::client::http_client client(U(global::search::meiliURL));
				// 创建第一个HTTP请求, 添加匹配索引
				web::http::http_request request_add_index(web::http::methods::POST);
				request_add_index.set_request_uri("/indexes/"s + global::search::orzmicSearchNamespace + "/search"s);
				request_add_index.headers().add("Content-Type", "application/json");
				request_add_index.headers().add("Authorization", "Bearer "s + global::search::meiliAuth);
				request_add_index.set_body(web::json::value::parse(body.dump()));

				auto response = client.request(request_add_index).get();

				if (response.status_code() < 400 and response.status_code() >= 200) {
					resp = json::parse(response.extract_json().get().serialize());
				} else {
					auto error = response.extract_string().get();
					throw self::HTTPException(error, 500);
				}

				// 遍历索引
				for (auto& index : resp["hits"]) {
					json data;

					if (showRankingScore) data["RankingScore"] = index.at("_rankingScore").get<float>();

					data["MusicId"] = index.at("MusicId").get<int32_t>();

					data["FileName"] = index.at("FileName").get<std::string>();
					data["Title"] = index.at("Title").get<std::string>();

					if (index.count("Aliases")) {
						data["Aliases"] = index.at("Aliases");
					}
					else {
						data["Aliases"] = json::array();
					}

					result.emplace_back(data);
				}
				if (result.is_null()) result = json::parse("[]");

				LogSystem::logInfo("[Orzmic]曲目匹配完成");
				return result.dump();
				});
			});
	}
}

/*
self::router::OrzmicRouter::~OrzmicRouter(){

}
*/