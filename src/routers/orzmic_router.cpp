#include "routers/orzmic_router.hpp"
#include "common/prevent_inject.hpp"

void self::router::OrzmicRouter::replaceStr(std::string& str){
	self::common::utils::replaceStrAll(str, "\"", "\"\"");
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

void self::router::OrzmicRouter::router(){
	CROW_ROUTE((*m_app), "/api/getMusic").methods(crow::HTTPMethod::Post)([&](const crow::request& req) {
		return self::HandleResponseBody([&] {
			LogSystem::logInfo("[Orzmic]曲目资源获取");
			auto accessControl{ getAccessControl(req.get_header_value("Authorization")) };
			if (accessControl.authority < 1){
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

				if(field_index > (where_field_names.size() - 1)) throw self::HTTPException("index out of bounds", 400);

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
				} else throw self::HTTPException("", 500);
			}elif(mode == 1){
				/* 获取标题匹配资源 */
				front_sql += "where title like \"%" + key + "%\"";
			}elif(mode == 2){ /* 获取全部资源 */ }
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
					if (index == 0){
						if (chart_designer_easy) music["Difficulties"][index]["ChartDesigner"] = *chart_designer_easy;
						if (difficulty_easy) music["Difficulties"][index]["Difficulty"] = *difficulty_easy;
						if (note_count_easy) music["Difficulties"][index]["NoteCount"] = *note_count_easy;
						if (rating_easy) music["Difficulties"][index]["Rating"] = *rating_easy;
					} elif (index == 1){
						if (chart_designer_normal) music["Difficulties"][index]["ChartDesigner"] = *chart_designer_normal;
						if (difficulty_normal) music["Difficulties"][index]["Difficulty"] = *difficulty_normal;
						if (note_count_normal) music["Difficulties"][index]["NoteCount"] = *note_count_normal;
						if (rating_normal) music["Difficulties"][index]["Rating"] = *rating_normal;
					} elif (index == 2){
						if (chart_designer_hard) music["Difficulties"][index]["ChartDesigner"] = *chart_designer_hard;
						if (difficulty_hard) music["Difficulties"][index]["Difficulty"] = *difficulty_hard;
						if (note_count_hard) music["Difficulties"][index]["NoteCount"] = *note_count_hard;
						if (rating_hard) music["Difficulties"][index]["Rating"] = *rating_hard;
					} elif (index == 3){
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
}

/*
self::router::OrzmicRouter::~OrzmicRouter(){

}
*/