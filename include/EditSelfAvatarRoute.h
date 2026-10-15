#pragma once
#include "EditSelfAvatar.h"
#include "httplib.h"
#include "json.hpp"
namespace self_avatar {
template<class Editor> inline void Register(httplib::Server& server,Editor editor){
 server.Post("/api/upload_head_img",[editor](const httplib::Request& req,httplib::Response& res){
  Result result;auto body=nlohmann::json::parse(req.body,nullptr,false);
  if(!body.is_object()||!body.contains("filepath")||!body["filepath"].is_string())result.message="filepath must be a string in a JSON object";
  else result=editor(body["filepath"].template get<std::string>());
  const bool success=result.ret==0&&result.uploaded&&result.verified;
  res.set_content(nlohmann::json({{"ret",success?0:-1},{"retmsg",result.message},{"errCode",success?1:-1},{"errMsg",result.message},
    {"submitted",result.submitted},{"uploaded",result.uploaded},{"verified",success},{"total_bytes",result.total},{"acknowledged_bytes",result.acknowledged}}).dump(),"application/json");
 });
}
inline void Register(httplib::Server& server){Register(server,[](const std::string& path){return Upload(path);});}
}
