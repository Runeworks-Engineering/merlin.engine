#include "glpch.h"
#include "Console.h"
#include <glad/glad.h>

#include <iostream>

namespace Merlin {
	std::string Console::endl = "\n";

	ConsoleLevel Console::_level = ConsoleLevel::_INFO;

	ConsoleStream::ConsoleStream(ConsoleLevel level, std::string origin) {
		_level = level;
		if (level >= Console::GetLevel()) {
			if (origin == "") Console::write(getFont(level) + getHeader(level) + endFont());
			else Console::write(getFont(level) + "[" + origin + "]: " + endFont());
		}
	}

	void Console::ClearGLError() {
		while (glGetError() != GL_NO_ERROR);
	}

	void Console::LogGLError() {
		while (GLenum error = glGetError()) {
			Console::error("OpenGL Error") << "(" << error << ")" << Console::endl;
		}
	}

	void Console::CheckGLError(std::string file, int line) {
		while (GLenum error = glGetError()) {
			Console::error("OpenGL Error") << "(" << error << ")" << " at line " << line << " in " << file << Console::endl;
		}
	}


	std::string Console::alignLeft(const int n, const std::string& x) { // converts x to string with spaces behind such that length is n if x is not longer than n
		std::string s = x;
		for (int i = 0u; i < n; i++) s += " ";
		return s.substr(0, max(n, (int)x.length()));
	}

	std::string Console::alignRight(const int n, const std::string& x) { // converts x to string with spaces in front such that length is n if x is not longer than n
		std::string s = "";
		for (int i = 0u; i < n; i++) s += " ";
		s += x;
		return s.substr((int)min((int)s.length() - (int)n, (int)n), s.length());
	}

	ConsoleStream Console::trace(std::string origin) {
		return ConsoleStream(ConsoleLevel::_TRACE, origin);
	}

	ConsoleStream Console::info(std::string origin) {
		return ConsoleStream(ConsoleLevel::_INFO, origin);
	}

	ConsoleStream Console::warn(std::string origin) {
		return ConsoleStream(ConsoleLevel::_WARNING, origin);
	}

	ConsoleStream Console::error(std::string origin) {
		return ConsoleStream(ConsoleLevel::_ERROR, origin);
	}

	ConsoleStream Console::success(std::string origin) {
		return ConsoleStream(ConsoleLevel::_SUCCESS, origin);
	}

	ConsoleStream Console::critical(std::string origin) {
		return ConsoleStream(ConsoleLevel::_CRITICAL, origin);
	}

	ConsoleStream Console::print(){
		return ConsoleStream(ConsoleLevel::_NO_HEADER, "");
	}

	void Console::printProgress(double percentage){
		int val = (int)(percentage * 100);
		int lpad = (int)(percentage * PBWIDTH);
		int rpad = PBWIDTH - lpad;
		printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");

		fflush(stdout);
	}

	void Console::write(std::string str) {
		printf(str.c_str());
	}


	std::string ConsoleStream::getHeader(ConsoleLevel lv) {
		std::string str;

		switch (lv) {
		case ConsoleLevel::_TRACE:
			str = "[Trace]: ";
			break;
		case ConsoleLevel::_INFO:
			str = "[Info]: ";
			break;
		case ConsoleLevel::_WARNING:
			str = "[Warning]: ";
			break;
		case ConsoleLevel::_ERROR:
			str = "[Error]: ";
			break;
		case ConsoleLevel::_SUCCESS:
			str = "[OK]: ";
			break;
		case ConsoleLevel::_CRITICAL:
			str = "[Fatal]: ";
			break;
		default:
			str = "";
			break;
		}
		return(str);
	}

	std::string ConsoleStream::getFont(ConsoleLevel lv) {
		std::string errorHtml = "\x1B[31m";
		std::string infoHtml = "\x1B[37m";
		std::string warnHtml = "\x1B[33m";
		std::string successHtml = "\x1B[32m";
		std::string str;

		switch (lv)
		{
		case ConsoleLevel::_CRITICAL:
			str = errorHtml;
			break;
		case ConsoleLevel::_ERROR:
			str = errorHtml;
			break;
		case ConsoleLevel::_WARNING:
			str = warnHtml;
			break;
		case ConsoleLevel::_INFO:
			str = infoHtml;
			break;
		case ConsoleLevel::_SUCCESS:
			str = successHtml;
			break;
		default:
			str = infoHtml;
			break;
		}

		return(str);
	}

	ConsoleStream ConsoleStream::operator<<(char i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += i + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(signed short i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += std::to_string(i) + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(unsigned short i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += std::to_string(i) + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(signed int i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += std::to_string(i) + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(unsigned int i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += std::to_string(i) + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(signed long i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += std::to_string(i) + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(unsigned long i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += std::to_string(i) + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(size_t i){
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += std::to_string(i) + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}


	ConsoleStream ConsoleStream::operator<<(float i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += std::to_string(i) + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(double i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += std::to_string(i) + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(const std::string& i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += i + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}


	ConsoleStream ConsoleStream::operator<<(const char* i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			str += i + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(const void* i) {
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			std::stringstream address;
			address << i;
			str += address.str() + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

	ConsoleStream ConsoleStream::operator<<(std::ostream os){
		if (_level >= Console::GetLevel()) {
			std::string str = this->font();
			std::stringstream ss;
			ss << os.rdbuf();
			str += ss.str() + ConsoleStream::endFont();

			Console::write(str);
		}
		return ConsoleStream(*this);
	}

}