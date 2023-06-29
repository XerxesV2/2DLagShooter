#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <deque>
#include <vector>
#include <chrono>
#include <string>
#include <memory>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00	//win 10
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
