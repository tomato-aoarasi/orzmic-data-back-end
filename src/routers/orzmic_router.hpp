#pragma once

#ifndef __CXX20_ORZMIC_ROUTER_HPP
#define __CXX20_ORZMIC_ROUTER_HPP

#include <memory>
#include "configuration/config.hpp"
#include "opencv2/opencv.hpp"

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