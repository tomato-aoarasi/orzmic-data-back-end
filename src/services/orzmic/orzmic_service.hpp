#pragma once

#ifndef __CXX20_ORZMIC_SERVICE_HPP
#define __CXX20_ORZMIC_SERVICE_HPP

#define __ORZMIC_SQL_ARGS int32_t music_id, std::string file_name, std::string title,int32_t lock_hint, bool initial_unlock, bool watermark,std::string artist, std::string cover_painter, std::string bpm,uint32_t audio_preview_from, uint32_t audio_preview_to,std::unique_ptr<std::string> chart_designer_easy, std::unique_ptr<std::string> difficulty_easy, std::unique_ptr<int> note_count_easy, std::unique_ptr<double> rating_easy,std::unique_ptr<std::string> chart_designer_normal, std::unique_ptr<std::string> difficulty_normal, std::unique_ptr<int> note_count_normal, std::unique_ptr<double> rating_normal,std::unique_ptr<std::string> chart_designer_hard, std::unique_ptr<std::string> difficulty_hard, std::unique_ptr<int> note_count_hard, std::unique_ptr<double> rating_hard,std::unique_ptr<std::string> chart_designer_special, std::unique_ptr<std::string> difficulty_special, std::unique_ptr<int> note_count_special, std::unique_ptr<double> rating_special, std::string extra_content

namespace services::orzmic {
	inline void analyzeData(json& data, const json& decodeJson, std::string_view addName) {
		constexpr int OVER_SCORE{ 1000000 }, S_SCORE{ 980000 }, A_SCORE{ 950000 }, B_SCORE{ 900000 }, FAILURE_SCORE{ 700000 };
		constexpr double MINIMUM_STANDARD_RATING{ 2.0 };
		constexpr std::uint8_t GOLDEN{ 0 }, SILVER{ 1 }, NONE{ 2 };
		constexpr std::uint8_t EZ{ 0 }, NR{ 1 }, HD{ 2 }, SP{ 3 };
		constexpr std::uint8_t CLEAR_TYPE_THEORETICAL_VALUE{ 0 }, CLEAR_TYPE_Z{ 1 }, CLEAR_TYPE_OVER_SCORE{ 2 }, CLEAR_TYPE_FULL_COMBO{ 3 }, CLEAR_TYPE_CLEAR{ 4 };
		for (auto& scoreJson : decodeJson) {
			std::istringstream issScores{ scoreJson.get<std::string>() + ",2" };

			std::vector<int> items;
			{
				std::string item;
				while (std::getline(issScores, item, ',')) {
					items.push_back(std::stoi(item));
				}
			}
			int musicId{ items.at(0) }, // 对应的是曲目MusicID
				difficulty{ items.at(1) }, // 0: EZ, 1: NR, 2:HD, 3:SP
				score{ items.at(2) }, // 单曲得分
				clearType{ items.at(3) }, // 0: 理论,1: 严判歌没有理论(Z),2: 超分(>=1000000),3 FC, 4 Clear
				plusType{ items.at(4) }; // 0: golden plus, 1: silver plus, 2: not plus
			// println(musicId, difficulty, score, clearType, plusType);

			global::db::orzmic << "select * from songs where music_id = ?;" << musicId
				>> [&](__ORZMIC_SQL_ARGS) {
				std::string evaluate{ "F" }, plusEvaluate{ "None" }, clearState{ "Clear" };
				int evaluateType{ 9 }, clearStateType{ 2 };
				json info{
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
					{"AudioPreviewTo", audio_preview_to},
					{"ChartDesigner", nullptr},
					{"Difficulty", nullptr},
					{"NoteCount", nullptr},
					{"Rating", nullptr}
				};
				double rating, rate{ 0.0 };

				if (difficulty == EZ) {
					if (chart_designer_easy) info["ChartDesigner"] = *chart_designer_easy;
					if (difficulty_easy) info["Difficulty"] = *difficulty_easy;
					if (note_count_easy) info["NoteCount"] = *note_count_easy;
					if (rating_easy) rating = *rating_easy;
				} elif(difficulty == NR) {
					if (chart_designer_normal) info["ChartDesigner"] = *chart_designer_normal;
					if (difficulty_normal) info["Difficulty"] = *difficulty_normal;
					if (note_count_normal) info["NoteCount"] = *note_count_normal;
					if (rating_normal) rating = *rating_normal;
				} elif(difficulty == HD) {
					if (chart_designer_hard) info["ChartDesigner"] = *chart_designer_hard;
					if (difficulty_hard) info["Difficulty"] = *difficulty_hard;
					if (note_count_hard) info["NoteCount"] = *note_count_hard;
					if (rating_hard) rating = *rating_hard;
				} elif(difficulty == SP) {
					if (chart_designer_special) info["ChartDesigner"] = *chart_designer_special;
					if (difficulty_special) info["Difficulty"] = *difficulty_special;
					if (note_count_special) info["NoteCount"] = *note_count_special;
					if (rating_special) rating = *rating_special;
				}
				info["Rating"] = rating;

				if (clearType == CLEAR_TYPE_THEORETICAL_VALUE) {
					rate += rating + 2.2;
				} elif(score > OVER_SCORE) {
					rate += rating + 2.1;
				} elif(score == OVER_SCORE) {
					rate += rating + 2.0;
				} elif(score >= S_SCORE) {
					constexpr int DIVISOR{ 20000 };
					rate += rating + 1.0 + static_cast<double>(static_cast<double>(score - S_SCORE) / DIVISOR);
				} elif(score >= A_SCORE) {
					constexpr int DIVISOR{ 50000 };
					rate += rating + 0.4 + static_cast<double>(static_cast<double>(score - A_SCORE) / DIVISOR);
				} elif(score >= B_SCORE) {
					constexpr int DIVISOR{ 125000 };
					rate += rating + static_cast<double>(static_cast<double>(score - B_SCORE) / DIVISOR);
				} elif(score >= FAILURE_SCORE and self::common::utils::compareDecimal(rating, MINIMUM_STANDARD_RATING) >= 0) {
					constexpr int DIVISOR{ 100000 };
					rate += rating - MINIMUM_STANDARD_RATING + static_cast<double>(static_cast<double>(score - FAILURE_SCORE) / DIVISOR);
				} elif(score >= FAILURE_SCORE) {
					constexpr int DIVISOR{ 200000 };
					rate += rating + static_cast<double>(static_cast<double>(score - FAILURE_SCORE) / DIVISOR);
				}

				// golden and silver plus ++
				if (plusType == GOLDEN) {
					plusEvaluate = "Golden";
					if (score >= OVER_SCORE)rate += 0.1;
					else rate += 0.05;
				} elif(plusType == SILVER) {
					plusEvaluate = "Silver";
					if (score >= OVER_SCORE)rate += 0.05;
					else rate += 0.02;
				}
				/*
				elif(plusType == NONE) {
					plusEvaluate = "None";
					rate += 0.0;
				}
				*/

				if (clearType <= CLEAR_TYPE_OVER_SCORE){
					clearState = "Over Score";
					clearStateType = 0;
				} elif(clearType == CLEAR_TYPE_FULL_COMBO) {
					clearState = "Full Combo";
					clearStateType = 1;
				}/*
				 else {
					clearState = "Clear";
					clearStateType = 2;
				 }
				 */

				constexpr int GRADE_S{ S_SCORE }, GRADE_A{ A_SCORE }, GRADE_B{ B_SCORE }, GRADE_C{ 850000 }, GRADE_D{ 800000 };
				if (clearType == CLEAR_TYPE_THEORETICAL_VALUE) {
					evaluateType = 0;
					evaluate = "ORZ";
				} elif(clearType == CLEAR_TYPE_Z) {
					evaluateType = 1;
					evaluate = "Z";
				} elif(score > OVER_SCORE) {
					evaluateType = 2;
					evaluate = "R";
				} elif(score == OVER_SCORE) {
					evaluateType = 3;
					evaluate = "O";
				} elif(score >= GRADE_S) {
					evaluateType = 4;
					evaluate = "S";
				} elif(score >= GRADE_A) {
					evaluateType = 5;
					evaluate = "A";
				} elif(score >= GRADE_B) {
					evaluateType = 6;
					evaluate = "B";
				} elif(score >= GRADE_C) {
					evaluateType = 7;
					evaluate = "C";
				} elif(score >= GRADE_D) {
					evaluateType = 8;
					evaluate = "D";
				}
				/*
				else {
					evaluateType = 9;
					evaluate = "F";
				}
				*/

				info["ClearState"] = clearState;
				info["ClearStateType"] = clearStateType;
				info["Evaluate"] = evaluate;
				info["EvaluateType"] = evaluateType;
				info["PlusEvaluate"] = plusEvaluate;
				info["PlusType"] = plusType;
				info["Score"] = score;
				info["ClearType"] = clearType;
				info["Rate"] = rate;
				info["Level"] = difficulty;
				
				std::string LevelNameStr{};
				json LevelName{ json::parse(extra_content).at("SpecialLevel").at(difficulty) };
				if (not LevelName.is_null()){
					if (difficulty == EZ) {
						LevelNameStr = "EZ";
					} elif (difficulty == NR) {
						LevelNameStr = "NR";
					} elif (difficulty == HD) {
						LevelNameStr = "HD";
					} elif (difficulty == SP) {
						LevelNameStr = "SP";
					}
				} else {
					LevelNameStr = LevelName.get<std::string>();
				}
				
				info["LevelName"] = LevelNameStr;
				data[addName].push_back(info);
			};
		}
	}

	inline int getLevelNum(std::string level) {
		int levelNum{ -1 };
		if (level == "easy") {
			levelNum = 0;
		} elif(level == "normal") {
			levelNum = 1;
		} elif(level == "hard") {
			levelNum = 2;
		} elif(level == "special") {
			levelNum = 3;
		}
		else {
			levelNum = -1;
		}
		return levelNum;
	};
}

#endif // !__CXX20_ORZMIC_SERVICE_HPP