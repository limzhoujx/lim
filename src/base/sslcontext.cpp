#include <lim/base/sslcontext.h>

namespace lim {
  SSLContext::SSLContext(bool is_client): is_verify_host_name_(false) {
    is_client_ = is_client;
		if (is_client)
			context_ = SSL_CTX_new(SSLv23_client_method());
		else
			context_ = SSL_CTX_new(SSLv23_server_method());
	}

  SSLContext::~SSLContext() {
    SSL_CTX_free(context_);
	}

	/**
	*SSL环境初始化
	* @return 失败返回false, 成功返回true
	*/
	bool SSLContext::InitEnviroment() {
		SSL_library_init(); 
    SSL_load_error_strings();
		OpenSSL_add_ssl_algorithms();
		return true;
	}

	/**
	*SSL环境销毁
	* @return 失败返回false, 成功返回true
	*/
	bool SSLContext::FreeEnviroment() {
		return true;
	}
	
	/**
	*加载CA证书
	* @param ca_file 包含一个或多个 PEM 格式的证书的文件的路径(必需)
	* @param ca_path 一个或多个 PEM 格式文件的路径，不过文件名必须使用特定的格式(可为空)
	* @return 失败返回false, 成功返回true
	*/
	bool SSLContext::LoadCALocations(const std::string &ca_file, const std::string &ca_path) {
		const char *char_ca_file = (ca_file.empty() ? NULL : ca_file.c_str());
		const char *char_ca_path = (ca_path.empty() ? NULL : ca_path.c_str());
		if (!SSL_CTX_load_verify_locations(context_, char_ca_file, char_ca_path))
			return false;
		else 
			return true;
	}

	/**
	*加载数字证书，用于发送给对端
	* @param certificate_file PEM 格式数字证书文件
	* @return 失败返回false, 成功返回true
	*/
	bool SSLContext::LoadCertificateFile(const std::string &certificate_file) {
		if (SSL_CTX_use_certificate_file(context_, certificate_file.c_str(), SSL_FILETYPE_PEM) <= 0)
			return false;
		else
			return true;
	}

	/**
	*加载私钥文件
	* @param private_key_file PEM 格式私钥文件
	* @return 失败返回false, 成功返回true
	*/
	bool SSLContext::LoadPrivateKeyFile(const std::string &private_key_file) {
		if (SSL_CTX_use_PrivateKey_file(context_, private_key_file.c_str(), SSL_FILETYPE_PEM) <= 0)
			return false;
		else
			return true;
	}

	/**
	*验证所加载的私钥和证书是否相匹配
	* @return 失败返回false, 成功返回true
	*/
	bool SSLContext::CheckPrivateKey() {
		if (!SSL_CTX_check_private_key(context_))
			return false;
		else
			return true;
	}

	/**
	*设置是否验证对端
	* @param is_verify_peer, 是否验证对端
	*/
	void SSLContext::SetVerifyPeer(int is_verify_peer) {
		if (is_verify_peer) {
			if (is_client_) {
				SSL_CTX_set_verify(context_, SSL_VERIFY_PEER, NULL);
			} else {
				SSL_CTX_set_verify(context_, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
			}
		} else {
			SSL_CTX_set_verify(context_, SSL_VERIFY_NONE, NULL);
		}
	}

  /**
  *是否校验HostName（只适用于Client模式）
  * @param is_verify_host_name 是否校验HostName
  */
  void SSLContext::SetVerifyHostName(bool is_verify_host_name) {
    if (is_client_) {
      is_verify_host_name_ = is_verify_host_name;
    }
  }

  bool SSLContext::IsVerifyHostName() {
    if (is_client_) {
      return is_verify_host_name_;
    } else {
      return false;
    }
  }
}

