
#include "response.hpp"
#include <string>
#include "../socket/socket.hpp"

std::string to_string(int num)
{
    std::stringstream stream;

    stream << num;

    std::string str;

    stream >> str;
    return str;
}

std::string response::get_error_page(int code)
{
    if(this->location.statusCode.count(code))
    {
        LocationData location = this->location;
        std::string             path;
        std::string             temp = location.statusCode[code];
        std::vector<char *>     list_locations;
        char                    *token = strtok ((char*)temp.c_str(),"/");
        struct stat             buff;

        // fill_tokens.
        if(token)
            list_locations.push_back(token);
        for (;token != NULL;)
        {
            token = strtok(NULL,"/");
            if(token != NULL)
                list_locations.push_back(token);       
        }
        path += "./";
        for (size_t i = 0; i < list_locations.size(); i++)
        {
            path += (std::string)list_locations[i] + "/";
        }
        path = path.substr(0, path.size()-1);
        if(lstat(path.c_str(),&buff) == 0)
            return path;
    }
    return "";
}

std::string response::get_body_res_page(int code)
{  
   std::string body = "<!DOCTYPE html>\n"\
"<html lang=\"en\">\n"\
"<head>\n"\
"    <meta charset=\"UTF-8\">\n"\
"    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n"\
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"\
"    <title>" + to_string(code) + " " + this->message_status[code] + "</title>\n"\
"    <style>\n"\
"        h1{text-align: center;}\n"\
"    </style>\n"\
"</head>\n"\
"<body>\n"\
"    <h1>" + to_string(code) + " " + this->message_status[code] +"</h1>\n"\
"    <hr>\n"\
"</body>\n"\
"</html>";
	return body;
}

std::string response::get_body_post(int code)
{
    std::string body = "<!DOCTYPE html>\n"\
"<html lang=\"en\">\n"\
"<head>\n"\
"    <meta charset=\"UTF-8\">\n"\
"    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n"\
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"\
"    <title>" + to_string(code) + " " + this->message_status[code] + "</title>\n"\
"    <style>\n"\
"        h1{text-align: center;}\n"\
"    </style>\n"\
"</head>\n"\
"<body>\n"\
"    <h1>" + to_string(code) + " " + this->message_status[code] +"</h1>\n"\
"    <hr>\n"\
"</body>\n"\
"</html>";
	return body;
}

void    response::set_response_page(int code)
{
    typedef struct data_headers{
        std::string content_length;
        std::string  content_type;
    } data_headers;
    typedef struct data_response{
        std::string     request_line;
        data_headers    headers;
        std::string     body;
    } data_response;

    std::string headers;
    data_response data;

    data.body = get_body_post(code);
    data.request_line = "HTTP/1.1 " + to_string(code) + " " + this->message_status[code];
    data.headers.content_length = to_string(data.body.length());
    data.headers.content_type = "text/html";
    headers+= data.request_line + "\r\n" +
            "Content-Length: " + data.headers.content_length + "\r\n" + "Content-Type: " + data.headers.content_type + 
               "\r\n\r\n";
    

	write(this->req.fd, headers.c_str(), headers.size());
	this->req.resFlag = HEADERSENT;
    write(this->req.fd, data.body.c_str(), data.body.size());
    this->req.resState = BODYSENT;
}

void    response::set_response_error(int code)
{
    typedef struct data_headers{
        std::string content_length;
        std::string  content_type;
    } data_headers;
    typedef struct data_response{
        std::string     request_line;
        data_headers    headers;
        std::string     body;
    } data_response;

    std::string headers;
    std::string error_page;
    data_response data;

    error_page = get_error_page(code);
    if(!error_page.length())
        data.body = get_body_res_page(code);
    
    data.request_line = "HTTP/1.1 " + to_string(code) + " " + this->message_status[code];
    if(error_page.length())
    {
        data.headers.content_length = get_content_length(error_page);
        data.headers.content_type = get_content_type(error_page);
    }
    else
    {
        data.headers.content_length = to_string(data.body.length());
        data.headers.content_type = "text/html";
    }

    headers+= data.request_line + "\r\n" +
               "Content-Length: " + data.headers.content_length + "\r\n" + "Content-Type: " + data.headers.content_type + 
               "\r\n\r\n";
	write(this->req.fd, headers.c_str(), headers.size());
	this->req.resFlag = HEADERSENT;
    if(error_page.length())
    {
	    this->req.fdBody = open(error_page.c_str(), O_RDONLY);
    }
    else
    {
        write(this->req.fd, data.body.c_str(), data.body.size());
        this->req.resState = BODYSENT;
    }
}

void    response::set_response_permanently(int code,std::string redirection = "")
{
    typedef struct data_headers{
        std::string location;
    } data_headers;
    typedef struct data_response{
        std::string     request_line;
        data_headers    headers;
        std::string     body;
    } data_response;

    std::string headers;
    data_response data;

    data.request_line = "HTTP/1.1 " + to_string(code) + " " + this->message_status[code];
    data.headers.location = "Location: http://" + this->req.headers["Host"];
    if(redirection.length())
        data.headers.location ="Location: " + redirection;
    else
        data.headers.location += this->req.headers["uri"] + "/";
    data.body = get_body_res_page(code);

    headers+= data.request_line + "\r\n" +
               data.headers.location +  "\r\n\r\n";
	write(this->req.fd, headers.c_str(), headers.size());
	this->req.resFlag = HEADERSENT;
    write(this->req.fd, data.body.c_str(), data.body.size());
    this->req.resState = BODYSENT;
}

std::string response::get_content_type(std::string path_file)
{
    std::string extention;

    extention = path_file.substr(path_file.find_last_of('.') + 1);
    if(this->content_types.count(extention))
        return(this->content_types[extention]);
    return "application/octet-stream";
}

std::string response::get_body(std::string path_file)
{
    std::ifstream file(path_file);
    std::string body;
	std::string	temp;

    if(file.is_open())
    {
        while (!file.eof())
		{
			std::getline(file, temp);
            body += temp;
		}
    }
    return body;
}

std::string response::get_content_length(std::string file)
{
    FILE* fp = fopen(file.c_str(), "r");

    if(fp != NULL)
    {
        fseek(fp, 0, SEEK_END);
        long int res = ftell(fp);
        fclose(fp);
        return to_string(res);
    }
    return "";
}

void       response::set_response_file(int code)
{
    typedef struct data_headers{
        std::string content_length;
        std::string  content_type;
    } data_headers;
    typedef struct data_response{
        std::string     request_line;
        data_headers    headers;
    } data_response;

    std::string headers;
    data_response data;

    data.request_line = "HTTP/1.1 " + to_string(code) + " " + this->message_status[code];
    data.headers.content_length = get_content_length(this->root);
    data.headers.content_type = get_content_type(this->root);
    
    headers+= data.request_line + "\r\n" +
               "Content-Length: " + data.headers.content_length + "\r\n" + "Content-Type: " + data.headers.content_type + 
               "\r\n\r\n";
	write(this->req.fd, headers.c_str(), headers.size());
	this->req.resFlag = HEADERSENT;
	this->req.fdBody = open(this->root.c_str(), O_RDONLY);
}

void    response::set_response_auto_index(int code,std::string body)
{
    typedef struct data_headers{
        std::string content_length;
        std::string  content_type;
    } data_headers;
    typedef struct data_response{
        std::string     request_line;
        data_headers    headers;
        std::string     body;
    } data_response;

    std::string headers;
    data_response data;
    
    data.body = body;
    data.request_line = "HTTP/1.1 " + to_string(code) + " " + this->message_status[code];
    data.headers.content_length = to_string(data.body.length());
    data.headers.content_type = "text/html";
    
    headers+= data.request_line + "\r\n" +
               "Content-Length: " + data.headers.content_length + "\r\n" + "Content-Type: " + data.headers.content_type + 
               "\r\n\r\n";
    write(this->req.fd, headers.c_str(), headers.size());
	this->req.resFlag = HEADERSENT;
    write(this->req.fd, data.body.c_str(), data.body.size());
    this->req.resState = BODYSENT;
}

void response::setMetaVariables()
{
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("SERVER_SOFTWARE", "Webserv", 1);
    setenv("PATH_TRANSLATED", this->root.c_str(), 1);
    setenv("SCRIPT_FILENAME", this->root.c_str(), 1);
    setenv("REDIRECT_STATUS", "200", 1);
    setenv("CONTENT_LENGTH", to_string(this->req.contentLen).c_str(), 1);
    setenv("CONTENT_TYPE", this->req.headers["Content-Type"].c_str(), 1);
    setenv("QUERY_STRING", this->req.headers["query"].c_str(), 1);

    setenv("REQUEST_METHOD", this->req.headers["method"].c_str(), 1);
    setenv("GATEWAY_INTERFACE","CGI/1.1",1);
    setenv("PATH_INFO", this->req.headers["uri"].c_str() , 1);
}

std::string	setCgiFileName(void)
{
	static unsigned int	count = 0;
	unsigned int		nb = ++count;
    std::string fileName = "/tmp/cgi";
    unsigned sz = fileName.size();
	for (; nb != 0; nb = nb / 10)
	    fileName.insert(sz, 1, nb % 10 + '0');
    return fileName;
}

std::string response::getCgiResponse()
{
    setMetaVariables();

    int pipeFd[2];
    int status;
    char *args[3] = {(char*)this->location.cgi.c_str(),(char*)this->root.c_str(), NULL};

    int cgiFileFd = open(this->req.fileName.c_str(), O_RDONLY);
    pipe(pipeFd);

    std::string cgiFileName = setCgiFileName();
    int pid = fork();
    if (pid == 0)
    {
        this->req.fdBody = open(cgiFileName.c_str(), O_WRONLY  | O_CREAT| O_TRUNC, 0644);

        dup2(this->req.fdBody, 1);
        dup2(cgiFileFd, 0);
        close(pipeFd[1]);
        execve(args[0], args, environ);
        // std::cerr << "ERROR EXECVE" << std::endl;
        exit(1);
    }
    else
    {
        close(pipeFd[1]);
        // waitpid(pid, &status, WNOHANG);
        wait(&status);// WNOHANG
        if (WEXITSTATUS(status) == 1) 
        {
            return "";
        }
        close(cgiFileFd);
        close(this->req.fdBody);
    }
    return cgiFileName;
}

int response::parseCgiOutput(std::string& fileName, response::Map& headers)
{
	std::string		buffer;
    std::ifstream   fileStream(fileName);
	std::getline(fileStream, buffer);

	while (buffer.compare("\r"))
	{
		parseCgiOutputHeader(buffer, headers);
		std::getline(fileStream, buffer);
	}
    std::streampos	bodyPos = fileStream.tellg();
	fileStream.seekg(0, std::ios_base::end);
    if (headers.count("Content-Length") == 0)
	    headers["Content-Length"] = to_string(fileStream.tellg() - bodyPos);
	fileStream.close();
    return bodyPos;
}

void	response::parseCgiOutputHeader(std::string& buffer, Map& headers)
{
	const size_t		lPos = buffer.size() - 1;
	const size_t		cPos = buffer.find(":", 0);
	const std::string	fieldName = buffer.substr(0, cPos);
	const std::string	fieldValue = buffer.substr(cPos + 2, lPos - cPos - 2);

	headers[fieldName] = fieldValue;
}

void    response::set_response_cgi()
{
    response::Map cgiHeaders;
    std::string cgiResponseFile = getCgiResponse();
    int position;

    typedef struct data_headers{
        std::string content_length;
        std::string  content_type;
    } data_headers;
    typedef struct data_response{
        std::string     request_line;
        data_headers    headers;
    } data_response;

    std::string headers;
    data_response data;


    if (cgiResponseFile.empty())
    {
        set_response_error(502);
        return;
    }
    position = parseCgiOutput(cgiResponseFile, cgiHeaders);

    data.request_line = "HTTP/1.1 200 OK";
    data.headers.content_length = cgiHeaders["Content-Length"];
    data.headers.content_type = cgiHeaders["Content-Type"];
    
    headers+= data.request_line + "\r\n" +
               "Content-Length: " + data.headers.content_length + "\r\n" + "Content-Type: " + data.headers.content_type + 
               "\r\n\r\n";
	write(this->req.fd, headers.c_str(), headers.size());
	this->req.resFlag = HEADERSENT;
    this->req.fdBody = open(cgiResponseFile.c_str(),O_RDONLY);
    char buff[position + 1];
    isFdReady(this->req.fdBody);
    read(this->req.fdBody, buff,position);
}



bool    response::request_valid(Request& req,long max_body_size)
{
    if(!req.headers.count("Host"))
    {
        set_response_error(400);
        return false;
    }
    if(req.headers["method"] != "GET" && req.headers["method"] != "POST" && req.headers["method"] != "DELETE")
    {
        set_response_error(501);
        return false;
    }

    if(!this->method_allowed(req.headers["method"]))
        return false;

    if(req.headers["method"] == "POST" && req.headers.count("Transfer-Encoding") && req.headers["Transfer-Encoding"] != "chunked")
    {
        set_response_error(501);
        return false;
    }
    if((req.headers["method"] == "POST" && !req.headers.count("Content-Length")) || (req.ActualContentLen != req.contentLen))
    {
        set_response_error(400);
        return false;
    }
    if(req.headers["uri"].find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=%") != std::string::npos)
	{
        set_response_error(400);
		return false;
	}
	if(req.headers["uri"].length() > 2048)
	{
        set_response_error(414);
		return false;
	}
    if((long)req.contentLen > max_body_size)
    {
        set_response_error(413);
        return false;
    }
    return true;
}


LocationData get_location(ServerData server, Request &my_request)
{
    if(!my_request.headers["uri"].length())
        my_request.headers["uri"] = "/";
    std::string             temp = my_request.headers["uri"];
    std::vector<char *>     list_locations;
    std::string             uri;
    size_t                  len;
    char                    *token = strtok ((char*)temp.c_str(),"/");
    LocationData            server_location(server);

    // fill_tokens.
    if(token)
        list_locations.push_back(token);
    for (;token != NULL;)
    {
        token = strtok(NULL,"/");
        if(token != NULL)
            list_locations.push_back(token);       
    }
    
    // choose location.
    
    len = list_locations.size();
    if(server.location.size())
    {
        while (len > 0)
        {
            for (size_t i = 0; i < len; i++)
                uri += "/" + (std::string)list_locations[i];
            for (size_t i = 0; i < server.location.size(); i++)
            {
                if(server.location[i].pathname == uri)
                    return server.location[i];
            }
            uri= "";
            len--;
        }
        for (size_t i = 0; i < server.location.size(); i++)
        {
            if(server.location[i].pathname == "/")
                return server.location[i];
        }
    }
    return server_location;
}

bool        response::check_location_config_file()
{
    if(!this->location.pathname.length())
    {
        set_response_error(404);
        return false;
    }
    if(this->location.redirect.second.length())
    {
        set_response_permanently(301,this->location.redirect.second);
        return false;
    }
    return true;
}

bool response::method_allowed(std::string method)
{
    for (size_t i = 0; i < this->location.acceptedMeths.size(); i++)
    {
        if(this->location.acceptedMeths[i] == method)
            return true;
    }
    set_response_error(405);
    return false;
}

bool    response::resource_root()
{
    std::string root = this->location.root;
    struct stat buff;
    if(root.length())
    {
        if(root[root.length() - 1] == '/')
            root = root.substr(0, root.size()-1);
        if(lstat((root + this->req.headers["uri"]).c_str() ,&buff) == 0)
        {
            this->root = root + this->req.headers["uri"];
            return true;
        }
    }
    set_response_error(404);
    return false;
}


bool    response::is_directory()
{
    struct stat buff;
    lstat(this->root.c_str(),&buff);
    if(S_ISDIR(buff.st_mode))
        return true;
    return false;
}

bool    response::is_slash_in_end()
{
    if(this->req.headers["uri"][this->req.headers["uri"].length() - 1] != '/')
    {
        set_response_permanently(301);
        return false;
    }
    return true;
}

bool    response::get_index_files()
{
    struct stat buff;
    std::string temp = this->root;
    
    if(temp[temp.length() - 1] != '/')
        temp += "/";
	for (size_t i = 0; i < this->location.index.size(); i++)
	{
		if(lstat((temp + this->location.index[i]).c_str() ,&buff) == 0)
		{
			this->root = temp + this->location.index[i];
			return true;
		}
	}
    return false;
}

bool    response::location_has_cgi()
{
    std::string extention;
    
    extention = this->root.substr(this->root.find_last_of('.') + 1);
    if(this->location.cgi.length() && extention == "php")
        return true;
    return false;
}

bool    response::post_location_has_cgi()
{
    if(location_has_cgi())
    {
        return true;
    }
    set_response_error(403);
    return false;
}

bool    response::is_auto_index()
{
    if(!this->location.autoindex)
    {
        set_response_error(403);
        return false;
    }
    return true;
}

std::string response::get_previous(std::string path)
{
         std::string             temp = path;
         std::string            new_path = "";
         std::vector<char *>     list_locations;
         char                    *token = strtok ((char*)temp.c_str(),"/");

        if(token)
            list_locations.push_back(token);
        for (;token != NULL;)
        {
            token = strtok(NULL,"/");
            if(token != NULL)
                list_locations.push_back(token);       
        }

        for(size_t i = 0; i < list_locations.size() - 1; i++)
        {
            new_path += "/" + (std::string)list_locations[i] + "/";
        }
    if(new_path.length())
    {
        new_path = new_path.substr(0, new_path.size()-1);
        return new_path;
    }
    return "/";
}

std::string response::get_auto_index_directory()
{
	std::string body;
	std::vector<std::string> list_files;
	std::string links;

	DIR *d;
    struct dirent *dir;

    d = opendir(this->root.c_str());
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
			if(strcmp(dir->d_name,".") != 0 && strcmp(dir->d_name,".git") != 0)
            {
                if(strcmp(dir->d_name,"..") == 0)
                {
                    if(this->location.root + "/" != this->root)
                        list_files.push_back(dir->d_name);
                }
                else
                    list_files.push_back(dir->d_name);
            }
        }
        closedir(d);
    }

	for (size_t i = 0; i < list_files.size(); i++)
	{
        std::string path = this->req.headers["Host"];
        if(list_files[i] == "..")
            path += get_previous(this->req.headers["uri"]);
        else
            path += this->req.headers["uri"] + list_files[i];
		links+= "    <a href= http://" + path +  ">" + list_files[i] + "</a><br>";
	}

	body = "<!DOCTYPE html>\n"\
"<html lang=\"en\">\n"\
"<head>\n"\
"    <meta charset=\"UTF-8\">\n"\
"    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n"\
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"\
"    <title>Auto Index</title>\n"\
"    <style>\n"\
"        h1{text-align: center;}\n"\
"    </style>\n"\
"</head>\n"\
"<body>\n"\
"<h1>Auto Index</h1>\n"\
"<hr>\n"\
+ links + "\n"\
"</body>\n"\
"</html>";

return body;
}

void       response::fill_content_types()
{
    this->content_types["html"] = "text/html";
	this->content_types["htm"] = "text/html";
	this->content_types["shtml"] = "text/html";
	this->content_types["css"] = "text/css";
	this->content_types["xml"] = "text/css";
	this->content_types["gif"]= "image/gif";
	this->content_types["jpeg"]= "image/jpeg";
	this->content_types["jpg"]= "image/jpeg";
	this->content_types["png"]= "image/png";
	this->content_types["js"]= "application/javascript";
	this->content_types["atom"]= "application/atom+xml";
	this->content_types["rss"]= "application/rss+xml";
	
	this->content_types["mml"]= "text/mathml";
	this->content_types["txt"]= "text/plain";
	this->content_types["jad"]= "text/vnd.sun.j2me.app-descriptor";
	this->content_types["wml"]= "text/vnd.wap.wml";
	this->content_types["htc"]= "text/x-component";
	
	this->content_types["png"]= "image/png";
	this->content_types["tif"]= "image/tiff";
	this->content_types["tiff"]= "image/tiff";
	this->content_types["wbmp"]= "image/vnd.wap.wbmp";
	this->content_types["ico"]= "image/x-icon";
	this->content_types["jng"]= "image/x-jng";
	this->content_types["bmp"]= "image/x-ms-bmp";
	this->content_types["svg"]= "image/svg+xml";
	this->content_types["svgz"]= "image/svg+xml";
	this->content_types["webp"]= "image/webp";

	this->content_types["woff"] = "application/font-woff";
	this->content_types["jar"] = "application/java-archive";
	this->content_types["war"] = "application/java-archive";
	this->content_types["ear"] = "application/java-archive";
	this->content_types["json"] = "application/json";
	this->content_types["hqx"] = "application/mac-binhex40";
	this->content_types["doc"] = "application/msword";
	this->content_types["pdf"] = "application/pdf";
	this->content_types["ps"] = "application/postscript";
	this->content_types["eps"] = "application/postscript";
	this->content_types["ai"] = "application/postscript";
	this->content_types["rtf"] = "application/rtf";
	this->content_types["m3u8"] = "application/vnd.apple.mpegurl";
	this->content_types["xls"] = "application/vnd.ms-excel";
	this->content_types["eot"] = "application/vnd.ms-fontobject";
	this->content_types["ppt"] = "application/vnd.ms-powerpoint";
	this->content_types["wmlc"] = "application/vnd.wap.wmlc";
	this->content_types["kml"] = "application/vnd.google-earth.kml+xml";
	this->content_types["kmz"] = "application/vnd.google-earth.kmz";
	this->content_types["7z"] = "application/x-7z-compressed";
	this->content_types["cco"] = "application/x-cocoa";
	this->content_types["jardiff"] = "application/x-java-archive-diff";
	this->content_types["jnlp"] = "application/x-java-jnlp-file";
	this->content_types["run"] = "application/x-makeself";
	this->content_types["pl"] = "application/x-perl";
	this->content_types["pm"] = "application/x-perl";
	this->content_types["prc"] = "application/x-pilot";
	this->content_types["pdb"] = "application/x-pilot";
	this->content_types["rar"] = "application/x-rar-compressed";
	this->content_types["rpm"] = "application/x-redhat-package-manager";
	this->content_types["sea"] = "application/x-sea";
	this->content_types["swf"] = "application/x-shockwave-flash";
	this->content_types["sit"] = "application/x-stuffit";
	this->content_types["tcl"] = "application/x-tcl";
	this->content_types["tk"] = "application/x-tcl";
	this->content_types["der"] = "application/x-x509-ca-cert";
	this->content_types["pem"] = "application/x-x509-ca-cert";
	this->content_types["cert"] = "application/x-x509-ca-cert";
	this->content_types["xpi"] = "application/x-xpinstall";
	this->content_types["xhtml"] = "application/xhtml+xml";
	this->content_types["xspf"] = "application/xspf+xml";
	this->content_types["zip"] = "application/zip";

	this->content_types["bin"] = "application/octet-stream";
	this->content_types["exe"] = "application/octet-stream";
	this->content_types["dll"] = "application/octet-stream";
	this->content_types["deb"] = "application/octet-stream";
	this->content_types["dmg"] = "application/octet-stream";
	this->content_types["iso"] = "application/octet-stream";
	this->content_types["img"] = "application/octet-stream";
	this->content_types["msi"] = "application/octet-stream";
	this->content_types["msp"] = "application/octet-stream";
	this->content_types["msm"] = "application/octet-stream";

	this->content_types["docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	this->content_types["xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	this->content_types["pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";

	this->content_types["mid"] = "audio/midi";
	this->content_types["midi"] = "audio/midi";
	this->content_types["kar"] = "audio/midi";
	this->content_types["mp3"] = "audio/mpeg";
	this->content_types["ogg"] = "audio/ogg";
	this->content_types["m4a"] = "audio/x-m4a";
	this->content_types["ra"] = "audio/x-realaudio";

	this->content_types["3gpp"] = "video/3gpp";
	this->content_types["3gp"] = "video/3gpp";
	this->content_types["ts"] = "video/mp2t";
	this->content_types["mp4"] = "video/mp4";
	this->content_types["mpeg"] = "video/mpeg";
	this->content_types["mpg"] = "video/mpeg";
	this->content_types["mov"] = "video/quicktime";
	this->content_types["webm"] = "video/webm";
	this->content_types["flv"] = "video/x-flv";
	this->content_types["m4v"] = "video/x-m4v";
	this->content_types["mng"] = "video/x-mng";
	this->content_types["asx"] = "video/x-ms-asf";
	this->content_types["asf"] = "video/x-ms-asf";
	this->content_types["wmv"] = "video/x-ms-wmv";
	this->content_types["avi"] = "video/x-msvideo";
}

bool response::index_files()
{
    if(get_index_files())
        return true;
    set_response_error(403);
    return false;
}

bool    response::support_upload()
{
    std::string upload_path = this->location.uploadPath;
    std::string file_name   = this->req.fileName;
	std::string	cmd;

    struct stat buff;

	this->root = this->location.root;
	if (this->root[this->root.size() - 1] == '/')
		this->root.erase(this->root.size() - 1, 1);
	this->root += this->req.headers["uri"];

    if(upload_path.length())
    {
        if(lstat(upload_path.c_str(),&buff) == 0 && S_ISDIR(buff.st_mode))
        {
			if (is_directory())
			{
            	cmd = "mv " + file_name + " " + upload_path;
            	system((cmd + " >nul 2>nul").c_str());
			}
			else
			{
            	cmd = "mv " + file_name + " " + this->root;
            	system((cmd + " >nul 2>nul").c_str());
            	cmd = "mv " + this->root + " " + upload_path;
            	system((cmd + " >nul 2>nul").c_str());
			}
            set_response_page(201);
            return true;
        }
        else
            set_response_error(403);
        }
    return false;
}

bool    response::is_slash_in_end_delete()
{
    if(this->req.headers["uri"][this->req.headers["uri"].length() - 1] != '/')
    {
        set_response_page(409);
        return false;
    }
    return true;
}

void    response::delete_folder()
{
    std::string cmd = "rm -rf " + this->root;
    if(access(this->root.c_str(),R_OK) == 0)
    {
        system((cmd + " >nul 2>nul").c_str());
        if(access(this->root.c_str(),F_OK) == 0)
            set_response_page(500);
        else
            set_response_page(204);
    }
    else
        set_response_page(403);
    return ;
}

void    response::delete_file()
{
    std::string cmd = "rm -rf " + this->root;
    system((cmd + " >nul 2>nul").c_str());
    set_response_page(204);
    return ;
}

void    response::DELETE_method()
{
    if(resource_root())
    {
        if(is_directory())
        {
            if(is_slash_in_end_delete())
            {
                if(location_has_cgi())
                {
                    if(index_files())
                    {
                        delete_file();
                    }
                }
                else
                    delete_folder();
            }
        }
        else
        {
            delete_file();
        }
    }
}

void    response::POST_method()
{
    if(!support_upload())
    {
        if(resource_root())
        {
            if(is_directory())
            {
                if(is_slash_in_end())
                {
                    if(index_files())
                    {
                        if(post_location_has_cgi())
                        {
                            set_response_cgi();
                        }
                    }
                }
            }
            else
            {
                if(post_location_has_cgi())
                {
                	set_response_cgi();
                }
            }
        }
    }
}

void    response::GET_method()
{
    if(resource_root())
    {
        if(is_directory())
        {
            if(is_slash_in_end())
            {
                if(get_index_files())
                {
                    if(location_has_cgi())
                    {
                        set_response_cgi();
                    }
                    else
                        set_response_file(200);
                }
                else
                {
                    if(is_auto_index())
                        set_response_auto_index(200,get_auto_index_directory());
                }
            }
        }
        else
        {
            if(location_has_cgi())
            {
                set_response_cgi();
            }
            else
                set_response_file(200);
        }
    }
}


void    handle_response(ServerData& server, Request& my_request)
{
    response res(get_location(server, my_request),my_request);
    if(res.request_valid(res.req, res.location.limitSize))
    {
        if(res.check_location_config_file())
        {
                if(res.req.headers["method"] == "GET")
                    res.GET_method();
                if(res.req.headers["method"] == "POST")
                    res.POST_method();
                if(res.req.headers["method"] == "DELETE")
                    res.DELETE_method();
        }
    }
}