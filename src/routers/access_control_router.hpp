#pragma once

#ifndef __CXX20_ACCESS_CONTROL_ROUTER_HPP
#define __CXX20_ACCESS_CONTROL_ROUTER_HPP

#include "configuration/config.hpp"

namespace self::router {
	class AccessControlRouter {
	private:
		std::shared_ptr<CrowApplication> m_app;
		define::AccessControl getAccessControl(std::string_view);
	public:
		explicit AccessControlRouter(std::shared_ptr<CrowApplication>);

		virtual void router();

		virtual ~AccessControlRouter() = default;
	};
}

#endif // !__CXX20_ACCESS_CONTROL_ROUTER_HPP