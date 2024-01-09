#pragma once

#ifndef __CXX20_MAIN_HPP
#define __CXX20_MAIN_HPP

#include "configuration/config.hpp"
#include "routers/orzmic_router.hpp"
#include "routers/access_control_router.hpp"

inline bool __init__{ [] {
	LogSystem::initialized();
	Config::initializeGlobalVariables();
	return true;
}() };

void init_router(std::shared_ptr<CrowApplication>);

inline void start() {
	auto app{ std::make_shared<CrowApplication>() };

	const auto& host{ global::server::host };
	const auto& port{ global::server::port };
	const auto& concurrency{ global::server::thread_num };

	LogSystem::logInfo(std::format("host: {} / port: {} / concurrency: {}", host, port, concurrency));

	// 日志等级
	crow::logger::setLogLevel(crow::LogLevel::INFO);

	app->bindaddr(host).port(port);

	init_router(app);

	if (concurrency <= 0){
		app->multithreaded().run_async();
	} else {
		app->concurrency(concurrency).run_async();
	}
};

inline void init_router(std::shared_ptr<CrowApplication> app){
	self::router::OrzmicRouter orzmic(app);
	orzmic.router();
	self::router::AccessControlRouter access(app);
	access.router();
}

#endif // !__CXX20_MAIN_HPP