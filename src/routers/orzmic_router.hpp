#pragma once

#ifndef __CXX20_ORZMIC_ROUTER_HPP
#define __CXX20_ORZMIC_ROUTER_HPP

#include <memory>
#include "configuration/config.hpp"
#include "opencv2/opencv.hpp"

#define __ORZMIC_SQL_ARGS int32_t music_id, std::string file_name, std::string title,int32_t lock_hint, bool initial_unlock, bool watermark,std::string artist, std::string cover_painter, std::string bpm,uint32_t audio_preview_from, uint32_t audio_preview_to,std::unique_ptr<std::string> chart_designer_easy, std::unique_ptr<std::string> difficulty_easy, std::unique_ptr<int> note_count_easy, std::unique_ptr<double> rating_easy,std::unique_ptr<std::string> chart_designer_normal, std::unique_ptr<std::string> difficulty_normal, std::unique_ptr<int> note_count_normal, std::unique_ptr<double> rating_normal,std::unique_ptr<std::string> chart_designer_hard, std::unique_ptr<std::string> difficulty_hard, std::unique_ptr<int> note_count_hard, std::unique_ptr<double> rating_hard,std::unique_ptr<std::string> chart_designer_special, std::unique_ptr<std::string> difficulty_special, std::unique_ptr<int> note_count_special, std::unique_ptr<double> rating_special, std::string extra_content

namespace self{
	namespace router {
		class OrzmicRouter
		{
		private:
			std::shared_ptr<CrowApplication> m_app;
			void replaceStr(std::string&);
			void updateSearch(int32_t);
			define::AccessControl getAccessControl(std::string_view);
			cv::Mat base64ToGrayScaleMat(std::string_view);
		public:
			explicit OrzmicRouter(std::shared_ptr<CrowApplication>);
			
			virtual void router();

			virtual ~OrzmicRouter() = default;
		};
	}
}

#endif // !__CXX20_ORZMIC_ROUTER_HPP