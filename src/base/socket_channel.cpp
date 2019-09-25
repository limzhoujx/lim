#include <lim/base/socket_channel.h>
#include <lim/base/string_utils.h>
#include <sstream>
#ifdef _WIN32 
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#endif
#ifdef ENABLE_OPENSSL
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#endif

namespace lim {
	SocketChannel::SocketChannel(int socket_channel): 
		socket_channel_(socket_channel), remote_host_port_(-1), local_host_port_(-1) {
		reference_count_ = new std::atomic<int>(1);
#ifdef ENABLE_OPENSSL		
		ssl_context_ = NULL;
		ssl_handle_ = NULL;
		is_handshaked_ = false;
#endif
	}

#ifdef ENABLE_OPENSSL	
	SocketChannel::SocketChannel(int socket_channel, SSLContext *ssl_context, SSL *ssl_handle):
		socket_channel_(socket_channel), remote_host_port_(-1), local_host_port_(-1), 
		ssl_context_(ssl_context), ssl_handle_(ssl_handle), is_handshaked_(false) {
		reference_count_ = new std::atomic<int>(1);
	}
#endif

	SocketChannel::SocketChannel(const SocketChannel &other)      {
		socket_channel_ = other.socket_channel_;
		
		remote_host_name_ = other.remote_host_name_;
		remote_host_port_ = other.remote_host_port_;
		
		local_host_name_ = other.local_host_name_;
		local_host_port_ = other.local_host_port_;

#ifdef ENABLE_OPENSSL	
		ssl_context_ = other.ssl_context_;
		ssl_handle_ = other.ssl_handle_;
		is_handshaked_ = other.is_handshaked_;
#endif		
		reference_count_ = other.reference_count_;
		(*reference_count_) ++;
	}

	SocketChannel &SocketChannel::operator =(const SocketChannel& other) {
    if (this == &other) 
      return *this;

    if(--(*reference_count_) <= 0) {
			Close();			
			delete reference_count_;	
    }

		socket_channel_ = other.socket_channel_;
		
		remote_host_name_ = other.remote_host_name_;
		remote_host_port_ = other.remote_host_port_;
		
		local_host_name_ = other.local_host_name_;
		local_host_port_ = other.local_host_port_;

#ifdef ENABLE_OPENSSL
		ssl_context_ = other.ssl_context_;
		ssl_handle_ = other.ssl_handle_;
		is_handshaked_ = other.is_handshaked_;
#endif

		reference_count_ = other.reference_count_;
		(*reference_count_) ++;
		
    return *this;
	}
	
	SocketChannel::~SocketChannel() {
		if (--(*reference_count_) <= 0) {
			Close();			
			delete reference_count_;	
		}
	}

	bool SocketChannel::InitEnviroment() {
#ifdef _WIN32 
		WSADATA  ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
      return false;
    }
#endif
    return true;
  }

  bool SocketChannel::FreeEnviroment() {
 #ifdef _WIN32 
    WSACleanup();
#endif
    return true;
  }
  
	bool SocketChannel::Bind(const std::string &unix_path) {
#ifndef _WIN32
		socket_channel_ = socket(AF_UNIX, SOCK_STREAM, 0);
		if (-1 == socket_channel_) {
			//printf("socket(AF_UNIX, SOCK_STREAM, 0) error.\n");
			return false;
		}

		int optval = 1;
		if (setsockopt(socket_channel_, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(int)) < 0) {
			//printf("setsockopt(SO_REUSEADDR) error.\n");
			Close();
			return false;
		}

		struct sockaddr_un local = {0};
		local.sun_family = AF_UNIX;
		strcpy(local.sun_path+1, unix_path.c_str());
    local.sun_path[0]=0;
	
		if (bind(socket_channel_, (sockaddr *)&local, sizeof(local)) < 0) {
			//printf("bind(%s) error.\n", unix_path.c_str());
			Close();
			return false;
		}

		if (listen(socket_channel_, 1024) < 0) {
			//printf("listen(%s) error.\n", unix_path.c_str());
			Close();
			return false;
		}

		if (!SetNonBlock()) {
			//printf("no block setting (%s) error.\n", unix_path.c_str());
			Close();
			return false;
		}
		
		return true;
#endif
		return false;
	}

	bool SocketChannel::Bind(const std::string &local_host, int local_port) {
		socket_channel_ = socket(AF_INET, SOCK_STREAM, 0);
		if (-1 == socket_channel_){
			//printf("socket(AF_INET, SOCK_STREAM, 0) error\n");
			return false;
		}

		int optval = 1;
		if (setsockopt(socket_channel_, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(int)) < 0) {
			//printf("setsockopt(SO_REUSEADDR) error.\n");
			Close();
			return false;
		}

		struct sockaddr_in local = {0};
		local.sin_family = AF_INET;
		local.sin_port = htons(local_port);
		local.sin_addr.s_addr = inet_addr(local_host.c_str());
		if (bind(socket_channel_, (sockaddr *)&local, sizeof(local)) < 0) {
			//printf("bind(%s, %d) error.\n", local_ip.c_str(), local_port);
			Close();
			return false;
		}

		if (listen(socket_channel_, 1024) < 0) {
			//printf("listen(%s, %d) error.\n", local_ip.c_str(), local_port);
			Close();
			return false;
		}

		if (!SetNonBlock()) {
			//printf("no block setting (%s:%d) error.\n", local_ip.c_str(), local_port);
			Close();
			return false;
		}	

		local_host_name_ = local_host;
		local_host_port_ = local_port;
		
		return true;
	}

	bool SocketChannel::Connect(const std::string &unix_path) {
#ifndef _WIN32
		socket_channel_ = socket(AF_UNIX, SOCK_STREAM, 0);
		if (-1 == socket_channel_) {
			//printf("socket(AF_UNIX, SOCK_STREAM, 0) error.\n");
			return false;
		}
		
		struct sockaddr_un remote = { 0 };
		remote.sun_family = AF_UNIX;
		strcpy(remote.sun_path + 1, unix_path.c_str());
		remote.sun_path[0] = 0;

		if (connect(socket_channel_, (sockaddr*)&remote, sizeof(remote)) < 0) {
 			//printf("connect(%s) error.\n", unix_path);
			Close();
			return false;
		}

		if (!SetNonBlock()) {
			//printf("no block setting (%s) error.\n", unix_path.c_str());
			Close();
			return false;
		}

#ifdef ENABLE_OPENSSL
    if (ssl_context_ != NULL) {
      ssl_handle_ = SSL_new(ssl_context_->context_);
      SSL_set_fd(ssl_handle_, socket_channel_);
      SSL_set_connect_state(ssl_handle_);
    }
#endif		
		return true;
#endif
		return false;
	}
	
	bool SocketChannel::Connect(const std::string &remote_host, int remote_port) {
		std::string remote_ip = HostToIp(remote_host);
		if (remote_ip.empty()) {
			return false;
		}
		
		socket_channel_ = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == socket_channel_) {
      //printf("socket(AF_INET, SOCK_STREAM, 0) error.\n");
      return false;
    }

    struct sockaddr_in remote = { 0 };
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(remote_ip.c_str());
    remote.sin_port = htons(remote_port);
    if (connect(socket_channel_, (sockaddr *)&remote, sizeof(remote)) < 0) {
      //printf("connect(%s:%d) error.\n", remote_ip.c_str(), remote_port);
      Close();
      return false;
    }

		if (!SetNonBlock()) {
			//printf("no block setting (%s:%d) error.\n", remote_ip.c_str(), remote_port);
			Close();
			return false;
		}
		
#ifdef ENABLE_OPENSSL
		if (ssl_context_ != NULL) {
			ssl_handle_ = SSL_new(ssl_context_->context_);
			SSL_set_fd(ssl_handle_, socket_channel_);
			SSL_set_connect_state(ssl_handle_);
		}
#endif	

		remote_host_name_ = remote_host;
		remote_host_port_ = remote_port;
		
    return true;
	}

	void SocketChannel::Accept(std::vector<SocketChannel> &socket_channels) {
		struct sockaddr_in remote_addr;
#ifdef _WIN32 
		int addr_length = sizeof(struct sockaddr_in);
#else
		socklen_t addr_length = sizeof(struct sockaddr_in);
#endif

		int accept_socket_channel;
		while ((accept_socket_channel = accept(socket_channel_, (struct sockaddr *)&remote_addr, &addr_length)) > 0) {
#ifdef ENABLE_OPENSSL
			if (ssl_context_ != NULL) {
				SSL *ssl_handle = SSL_new(ssl_context_->context_);
				SSL_set_fd(ssl_handle, accept_socket_channel);
				SSL_set_accept_state(ssl_handle);
			
				SocketChannel channel(accept_socket_channel, ssl_context_, ssl_handle);
				channel.SetNonBlock();
				socket_channels.push_back(channel);
			} else {
				SocketChannel channel(accept_socket_channel);
				channel.SetNonBlock();
				socket_channels.push_back(channel);
			}
#else
			SocketChannel channel(accept_socket_channel);
			channel.SetNonBlock();
			socket_channels.push_back(channel);
#endif
		}
	}
	
	bool SocketChannel::Close() {
		if (-1 == socket_channel_) {
      return false;
    }
		
#ifdef ENABLE_OPENSSL
		if (ssl_handle_ != NULL) {
			SSL_shutdown(ssl_handle_);
			SSL_free(ssl_handle_);
			ssl_handle_ = NULL;
		}
#endif	

#ifdef _WIN32
		shutdown(socket_channel_, SD_BOTH);
		closesocket(socket_channel_);
#else	
		shutdown(socket_channel_, SHUT_RDWR);
		close(socket_channel_);
#endif
		socket_channel_ = -1;
		return true;
	}

	std::string SocketChannel::HostToIp(const std::string &host_name) {
		addrinfo hints, *res;
		memset(&hints, 0, sizeof(addrinfo));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_INET;

		int ret = getaddrinfo(host_name.c_str(), NULL, &hints, &res);
		if (ret != 0) {
			//printf("getaddrinfo error %d : %s\n", ret, gai_strerror(ret));
			return "";
		}

		
		in_addr addr;
		addr.s_addr = ((sockaddr_in*)(res->ai_addr))->sin_addr.s_addr;

		char ip_addr[128] = {0};
		inet_ntop(AF_INET, (void *)&addr, ip_addr, sizeof(ip_addr));

		freeaddrinfo(res);

		return std::string(ip_addr);
	}
	
	bool SocketChannel::SetNonBlock() {
		if (-1 == socket_channel_) {
      return false;
    }
#ifdef _WIN32
    unsigned long flag = 1;
    if (ioctlsocket(socket_channel_, FIONBIO, &flag) == SOCKET_ERROR)
      return false;
#else
    int flag = fcntl(socket_channel_, F_GETFL);
    if (flag < 0)
      return false;

    flag |= O_NONBLOCK;
    if (fcntl(socket_channel_, F_SETFL, flag) < 0)
      return false;
#endif
    return true;
  }

#ifdef ENABLE_OPENSSL
	bool SocketChannel::SSLHandshake(int &waiton_flag) {
    waiton_flag = 0;
		if (-1 == socket_channel_ || ssl_context_ == NULL || ssl_handle_ == NULL) {
			return false;
		}
		
		int ret = SSL_do_handshake(ssl_handle_);
		if (ret == 1) { //握手成功
			is_handshaked_ = true;
			return true;
		}
		
		int error_code = SSL_get_error(ssl_handle_, ret);
    if (error_code == SSL_ERROR_WANT_READ) {
      waiton_flag = SSL_ERROR_WANT_READ;
      return true;
    } else if (error_code == SSL_ERROR_WANT_WRITE) {
      waiton_flag = SSL_ERROR_WANT_WRITE;
      return true;
    } else {
      return false;
    }
	}
	
	int SocketChannel::SSLReadBytes(ByteBuffer &buffer, int &waiton_flag) {
		waiton_flag = 0;
		if (-1 == socket_channel_ || ssl_context_ == NULL || ssl_handle_ == NULL) {
			return -1;
		}

		int total_read_length = 0;	
		const int kDefaultReadBufferSize = 1024;
		char read_buffer[kDefaultReadBufferSize] = { 0 };
		int read_buffer_size = (buffer.WritableBytes() > kDefaultReadBufferSize ? kDefaultReadBufferSize : buffer.WritableBytes());
		
		while (read_buffer_size > 0) {
			int read_length = SSL_read(ssl_handle_, read_buffer, read_buffer_size);
			if (read_length > 0) {
				total_read_length += read_length;
				buffer.WriteBytes(read_buffer, read_length);
				read_buffer_size = (buffer.WritableBytes() > kDefaultReadBufferSize ? kDefaultReadBufferSize : buffer.WritableBytes());
			} else {
				int error_code = SSL_get_error(ssl_handle_, read_length);
				if (error_code == SSL_ERROR_WANT_READ) {
					waiton_flag = SSL_ERROR_WANT_READ;
				} else if (error_code == SSL_ERROR_WANT_WRITE) {
					waiton_flag = SSL_ERROR_WANT_WRITE;
				} else {
					total_read_length = (total_read_length == 0 ? -1 : total_read_length);
				}
				break;
			}
		}
		return total_read_length;
	}

	int SocketChannel::SSLWriteBytes(ByteBuffer &buffer, int &waiton_flag) {
		waiton_flag = 0;
		if (-1 == socket_channel_ || ssl_context_ == NULL || ssl_handle_ == NULL) {
			return -1;
		}

		int total_write_length = 0;
		const int kDefaultWriteBufferSize = 1024;
		char write_buffer[kDefaultWriteBufferSize] = { 0 };
		int write_buffer_size = (buffer.ReadableBytes() > kDefaultWriteBufferSize ? kDefaultWriteBufferSize : buffer.ReadableBytes());
		buffer.ReadBytes(write_buffer, write_buffer_size);

		while (write_buffer_size > 0) {
			int write_length = SSL_write(ssl_handle_, write_buffer, write_buffer_size);
			if (write_length > 0) {
				total_write_length += write_length;
				write_buffer_size = (buffer.ReadableBytes() > kDefaultWriteBufferSize ? kDefaultWriteBufferSize : buffer.ReadableBytes());
				buffer.ReadBytes(write_buffer, write_buffer_size);
			} else {
				int error_code = SSL_get_error(ssl_handle_, write_length);
				if (error_code == SSL_ERROR_WANT_READ) {
					waiton_flag = SSL_ERROR_WANT_READ;
				} else if (error_code == SSL_ERROR_WANT_WRITE) {
					waiton_flag = SSL_ERROR_WANT_WRITE;
				} else {
					total_write_length = (total_write_length == 0 ? -1 : total_write_length);
				}
				break;
			}
		}
		return total_write_length;
	}

	bool SocketChannel::SSLCheckHostName() {
		if (-1 == socket_channel_ || ssl_context_ == NULL || ssl_handle_ == NULL) {
			return false;
		}
		
    //不需要验证HostName
    if (!ssl_context_->IsVerifyHostName()) {
      return true;
    }

    X509 *cert = SSL_get_peer_certificate(ssl_handle_);
    if (cert == NULL) {
			return false;
    }

    const char *peer_host_name = strstr(remote_host_name_.c_str(), ".");
    if (peer_host_name == NULL) {
      peer_host_name = remote_host_name_.c_str();
    }

    STACK_OF(GENERAL_NAME) *altnames = (STACK_OF(GENERAL_NAME) *)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
		if (altnames) {
			int num = sk_GENERAL_NAME_num(altnames);
			for (int i = 0; i < num; i++) {
				char *host_name = NULL;
				GENERAL_NAME *altname = sk_GENERAL_NAME_value(altnames, i);
				if (altname->type == GEN_DNS) {
					host_name = (char *) ASN1_STRING_data(altname->d.dNSName);
				} else if (altname->type == GEN_IPADD) {
					host_name = (char *) ASN1_STRING_data(altname->d.iPAddress);
				}

				if (host_name == NULL) {
					continue;
				}		

				if (strlen(host_name) > 2 && host_name[0] == '*' && host_name[1] == '.') {
					host_name += 1;
				}

				if (strncasecmp(host_name, peer_host_name, strlen(host_name)) == 0) {
					GENERAL_NAMES_free(altnames);
					X509_free(cert);
					return true;
				}
			}

			GENERAL_NAMES_free(altnames);
    }

    X509_NAME *sname = X509_get_subject_name(cert);
    if (sname == NULL) {
			X509_free(cert);
			return false;
    }

    int index = -1;
    for ( ;; ) {
			index = X509_NAME_get_index_by_NID(sname, NID_commonName, index);
			if (index < 0) {
 				break;
			}

			X509_NAME_ENTRY *entry = X509_NAME_get_entry(sname, index);
			char *host_name = (char *) ASN1_STRING_data(X509_NAME_ENTRY_get_data(entry));

			if (strlen(host_name) > 2 && host_name[0] == '*' && host_name[1] == '.') {
				host_name += 1;
			}

			if (strncasecmp(host_name, peer_host_name, strlen(host_name)) == 0) {
				X509_free(cert);
				return true;
			}
    }

		return false;
}
#endif
	
	int SocketChannel::ReadBytes(ByteBuffer &buffer) {
		if (-1 == socket_channel_) {
      return -1;
    }

		int total_read_length = 0;
		const int kDefaultReadBufferSize = 1024;
		char read_buffer[kDefaultReadBufferSize] = { 0 };
		int read_buffer_size = (buffer.WritableBytes() > kDefaultReadBufferSize ? kDefaultReadBufferSize : buffer.WritableBytes());

		while (read_buffer_size > 0) {
			int read_length = recv(socket_channel_, read_buffer, read_buffer_size, 0);
			if (read_length > 0) {
				total_read_length += read_length;
				buffer.WriteBytes(read_buffer, read_length);
				read_buffer_size = (buffer.WritableBytes() > kDefaultReadBufferSize ? kDefaultReadBufferSize : buffer.WritableBytes());
			} else if (read_length == 0) {//socket closed
				total_read_length = (total_read_length == 0 ? -1 : total_read_length);
				break;
			} else {
#ifndef _WIN32
				if (errno != EAGAIN)
#else
				if (GetLastError() != WSAEWOULDBLOCK)
#endif 
					total_read_length = (total_read_length == 0 ? -1 : total_read_length);

				break;
			}
		}
		
		return total_read_length;
	}

	int SocketChannel::WriteBytes(ByteBuffer &buffer) {
		if (-1 == socket_channel_) {
      return -1;
    }

		int total_write_length = 0;
		const int kDefaultWriteBufferSize = 1024;
		char write_buffer[kDefaultWriteBufferSize] = { 0 };
		int write_buffer_size = (buffer.ReadableBytes() > kDefaultWriteBufferSize ? kDefaultWriteBufferSize : buffer.ReadableBytes());
		buffer.ReadBytes(write_buffer, write_buffer_size);

		while (write_buffer_size > 0) {
			int write_length = send(socket_channel_, write_buffer, write_buffer_size, 0);
			if (write_length > 0) {
				total_write_length += write_length;
				write_buffer_size = (buffer.ReadableBytes() > kDefaultWriteBufferSize ? kDefaultWriteBufferSize : buffer.ReadableBytes());
				buffer.ReadBytes(write_buffer, write_buffer_size);
			} else if (write_length == 0) {//socket closed
				total_write_length = (total_write_length == 0 ? -1 : total_write_length);
				break;
			} else {
#ifndef _WIN32
				if (errno != EAGAIN)
#else
				if (GetLastError() != WSAEWOULDBLOCK)
#endif 
					total_write_length = (total_write_length == 0 ? -1 : total_write_length);

				break;
			}
		}

		return total_write_length;
	}

	std::string SocketChannel::ToString() {
		std::stringstream ss;
		ss << "channel_id: " << socket_channel_;
		
		if (!local_host_name_.empty()) {
			ss << ", local: " << local_host_name_ << ":" << local_host_port_;
		}
		
		if (!remote_host_name_.empty()) {
			ss << ", remote: " << remote_host_name_ << ":" << remote_host_port_;
    }

		return ss.str();
	}
}

