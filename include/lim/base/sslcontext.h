#ifndef LIM_SSL_CONTEXT_H
#define LIM_SSL_CONTEXT_H
#include <string>
#include <vector>
#include <stdint.h>
#include <openssl/ssl.h>

namespace lim {
	class SSLContext {
	public:
    SSLContext(bool is_client = false);
		virtual ~SSLContext();

		/**
		*SSL环境初始化
		* @return 失败返回false, 成功返回true
		*/
		static bool InitEnviroment();
		/**
		*SSL环境销毁
		* @return 失败返回false, 成功返回true
		*/
		static bool FreeEnviroment();
	
		/**
		*加载CA证书
		* @param ca_file 包含一个或多个 PEM 格式的证书的文件的路径(必需)
		* @param ca_path 一个或多个 PEM 格式文件的路径，不过文件名必须使用特定的格式(可为空)
		* @return 失败返回false, 成功返回true
		*/
		bool LoadCALocations(const std::string &ca_file, const std::string &ca_path);
		/**
		*加载数字证书，用于发送给对端
		* @param certificate_file PEM 格式数字证书文件
		* @return 失败返回false, 成功返回true
		*/
		bool LoadCertificateFile(const std::string &certificate_file);
		/**
		*加载私钥文件
		* @param private_key_file PEM 格式私钥文件
		* @return 失败返回false, 成功返回true
		*/
		bool LoadPrivateKeyFile(const std::string &private_key_file);
		/**
		*验证所加载的私钥和证书是否相匹配
		* @return 失败返回false, 成功返回true
		*/
		bool CheckPrivateKey();

		/**
		*设置是否验证对端
		* @param is_verify_peer, 是否验证对端
		*/
		void SetVerifyPeer(int is_verify_peer);

    /**
    *是否校验HostName（只适用于Client模式）
    * @param is_verify_host_name 是否校验HostName
    */
    void SetVerifyHostName(bool is_verify_host_name);
    bool IsVerifyHostName();

    bool IsClientContext() { return is_client_; };

	protected:
    bool is_client_;
		SSL_CTX	*context_;
    bool is_verify_host_name_;
    friend class SocketChannel;
	};
}
#endif

