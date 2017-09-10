//
// Created by saman on 14/08/17.
//

#ifndef NEWWEBSERVER_HTTP_H
#define NEWWEBSERVER_HTTP_H
namespace utserver{
struct HTTPHeaders{
    std::string _name;
    std::string _value;

    HTTPHeaders(std::string name, std::string value): _name(name), _value(value){};
};
}
#endif //NEWWEBSERVER_HTTP_H
