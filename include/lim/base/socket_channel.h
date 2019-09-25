#ifndef LIM_SOCKET_CHANNEL_H
#define LIM_SOCKET_CHANNEL_H
#include <lim/config.h>
#include <lim/base/byte_buffer.h>
#ifdef ENABLE_OPENSSL	
#include <lim/base/sslcontext.h>
#endif
#include <string>
#include <vector>
#include <atomic>

namespace lim {
	class SocketChannel {
	public:
		SocketChannel(int socket_channel = -1);
#ifdef ENABLE_OPENSSL	
		SocketChannel(int socket_channel, SSLContext *ssl_context, SSL *ssl_handle);
#endif
		SocketChannel(const SocketChannel& other);
    SocketChannel &operator=(const SocketChannel& other);
		virtual ~SocketChannel();
		
		static bool InitEnviroment();
		static bool FreeEnviroment();
		
		bool Bind(const std::string &unix_path);
		bool Bind(const std::string &local_host, int local_port);

		bool Connect(const std::string &unix_path);
		bool Connect(const std::string &remote_host, int remote_port);

		void Accept(std::vector<SocketChannel> &socket_channels);
		
		bool Close();
		
		bool SetNonBlock();
		
		std::string HostToIp(const std::string &host_name);

#ifdef ENABLE_OPENSSL		
		bool IsSSLChannel() { return (ssl_context_ != NULL); };
		bool IsSSLHandshaked() { return is_handshaked_; };
    bool SSLCheckHostName();

		void SetSSLContext(SSLContext *ssl_context) { ssl_context_ = ssl_context; };
    SSLContext *GetSSLContext() { return ssl_context_; };
#endif

		std::string ToString();

#ifdef ENABLE_OPENSSL		
		bool SSLHandshake(int &waiton_flag);
		int SSLReadBytes(ByteBuffer &buffer, int &waiton_flag);
		int SSLWriteBytes(ByteBuffer &buffer, int &waiton_flag);
#endif

		int ReadBytes(ByteBuffer &buffer);
		int WriteBytes(ByteBuffer &buffer);

		std::string GetRemoteHostName() { return remote_host_name_; }
		int GetRemoteHostPort() { return remote_host_port_; }

		std::string GetLocalHostName() { return local_host_name_; }
		int GetLocalHostPort() { return local_host_port_; }
		
	protected:
		int socket_channel_;
#ifdef ENABLE_OPENSSL		
    SSLContext *ssl_context_;
		SSL *ssl_handle_;
		bool is_handshaked_;
#endif
		std::string remote_host_name_; /***对端地址***/
		int remote_host_port_; /***对端端口***/

		std::string local_host_name_; /***本地地址***/
		int local_host_port_; /***本地端口***/
		
		std::atomic<int> *reference_count_;
		friend class EventLoop;
	};
}
#endif


