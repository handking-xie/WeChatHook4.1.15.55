const fs = require('fs');

const openapiPath = 'd:\\WeChat-Hook\\WeChat-Hook-openapi.json';
const doc = JSON.parse(fs.readFileSync(openapiPath, 'utf8'));

// Define new paths
const newPaths = {
  "/IsLoginStatus": {
    "post": {
      "summary": "检查登录状态",
      "description": "检查当前微信是否已经登录，并返回当前登录账号的基本信息（wxid、昵称、微信号）。",
      "requestBody": {
        "content": {
          "application/json": {
            "example": {}
          }
        }
      },
      "responses": {
        "200": {
          "description": "登录状态信息",
          "content": {
            "application/json": {
              "example": {
                "ret": 0,
                "status": 1,
                "is_login": true,
                "wxid": "wxid_sz5ipr263uwv12",
                "nickname": "测试用户",
                "alias": "my_alias"
              }
            }
          }
        }
      }
    }
  },
  "/GetContact": {
    "post": {
      "summary": "获取联系人/群聊信息",
      "description": "根据 wxid 或微信号查询好友、公众号或群聊的详细资料。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "wxid": "filehelper"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "联系人详情",
          "content": {
            "application/json": {
              "example": {
                "ret": 0,
                "retmsg": "success",
                "data": {
                  "username": "filehelper",
                  "alias": "",
                  "nick_name": "文件传输助手",
                  "remark": "",
                  "big_head_url": "",
                  "small_head_url": ""
                }
              }
            }
          }
        }
      }
    }
  },
  "/GetFriendOrChatroomDetailInfo": {
    "post": {
      "summary": "拉取好友或群详情",
      "description": "别名接口：与 /GetContact 相同，拉取好友或群详细资料。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "wxid": "25694009164@chatroom"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "群或好友详情"
        }
      }
    }
  },
  "/BatchGetContactBriefInfo": {
    "post": {
      "summary": "批量获取联系人简要信息",
      "description": "传入多个 wxid（支持数组或逗号分隔），批量获取昵称、头像、备注信息。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "wxidlist": "filehelper,wxid_sz5ipr263uwv12"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "联系人简要信息列表"
        }
      }
    }
  },
  "/GetHeadIMG": {
    "post": {
      "summary": "获取头像 URL",
      "description": "根据 wxid 查询联系人或群聊的高清大图头像和缩略图头像 URL。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "wxid": "filehelper"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "头像信息"
        }
      }
    }
  },
  "/GetContactLabelList": {
    "post": {
      "summary": "获取联系人标签列表",
      "description": "获取微信通信录中配置的所有好友标签（分组）。",
      "requestBody": {
        "content": {
          "application/json": {
            "example": {}
          }
        }
      },
      "responses": {
        "200": {
          "description": "标签列表"
        }
      }
    }
  },
  "/QueryChatRoomOwnerWxid": {
    "post": {
      "summary": "查询群主 wxid",
      "description": "根据群 ID (gid) 查询群主的 wxid。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "gid": "25694009164@chatroom"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "群主 wxid"
        }
      }
    }
  },
  "/BatchGetChatRoomMemberWxid": {
    "post": {
      "summary": "获取群成员 wxid 列表",
      "description": "查询指定群聊的所有群成员 wxid 列表。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "gid": "25694009164@chatroom"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "群成员列表",
          "content": {
            "application/json": {
              "example": {
                "ret": 0,
                "gid": "25694009164@chatroom",
                "data": [
                  { "wxid": "wxid_sjx2weyau5jh22" },
                  { "wxid": "meimeishisan" }
                ]
              }
            }
          }
        }
      }
    }
  },
  "/QueryChatRoomMemberCount": {
    "post": {
      "summary": "查询群成员数量",
      "description": "快速查询指定群聊中的成员总人数。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "gid": "25694009164@chatroom"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "成员数量",
          "content": {
            "application/json": {
              "example": {
                "ret": 0,
                "gid": "25694009164@chatroom",
                "count": "233"
              }
            }
          }
        }
      }
    }
  },
  "/QueryChatRoomMemberNickName": {
    "post": {
      "summary": "查询群成员群昵称",
      "description": "查询指定成员在群聊中的显示昵称/备注名。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "gid": "25694009164@chatroom",
              "wxid": "wxid_sjx2weyau5jh22"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "成员群昵称",
          "content": {
            "application/json": {
              "example": {
                "ret": 0,
                "nickname": "鸡丁妈妈"
              }
            }
          }
        }
      }
    }
  },
  "/GetChatrooMmemberDetail": {
    "post": {
      "summary": "获取群成员详情列表",
      "description": "获取指定群聊的完整详情，包含群名称、群主信息、成员总数以及所有成员的头像、昵称列表。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "gid": "25694009164@chatroom"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "群详情及成员列表",
          "content": {
            "application/json": {
              "example": {
                "ret": 0,
                "type": "chatroom",
                "gid": "25694009164@chatroom",
                "member_count": "5",
                "master_wxid": "honglin6974",
                "master_nickname": "林林",
                "chatroom_name": "吃货物资投喂群",
                "member": [
                  {
                    "wxid": "wxid_sjx2weyau5jh22",
                    "nickname": "鸡丁妈妈",
                    "user_head_big": "https://wx.qlogo.cn/...",
                    "user_head_small": "https://wx.qlogo.cn/..."
                  }
                ]
              }
            }
          }
        }
      }
    }
  },
  "/GetChatroomMemberDetailInfo": {
    "post": {
      "summary": "查询群成员详细资料",
      "description": "查询群内特定成员的完整资料信息。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "gid": "25694009164@chatroom",
              "wxid": "wxid_sjx2weyau5jh22"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "群成员资料"
        }
      }
    }
  },
  "/InitContact": {
    "post": {
      "summary": "初始化通讯录",
      "description": "获取当前通讯录中的全部联系人和群聊 ID 列表。",
      "requestBody": {
        "content": {
          "application/json": {
            "example": {
              "contactSeq": 0,
              "chatRoomContactSeq": 0
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "通讯录 ID 列表"
        }
      }
    }
  },
  "/QueryBodyInfo": {
    "post": {
      "summary": "按关键词查询联系人基础资料",
      "description": "通过关键词（微信号、昵称、备注或手机号）搜索匹配的联系人资料。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "serachwhat": "文件传输助手"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "匹配的联系人信息"
        }
      }
    }
  },
  "/GetSessionList": {
    "post": {
      "summary": "获取会话列表",
      "description": "读取微信最近会话列表（包括私聊和群聊），包含最后一条消息预览、未读数、时间戳等。",
      "requestBody": {
        "content": {
          "application/json": {
            "example": {}
          }
        }
      },
      "responses": {
        "200": {
          "description": "会话列表"
        }
      }
    }
  },
  "/GetFavList": {
    "post": {
      "summary": "获取收藏列表",
      "description": "获取微信收藏夹中的收藏项列表（ID、类型、时间）。",
      "requestBody": {
        "content": {
          "application/json": {
            "example": {}
          }
        }
      },
      "responses": {
        "200": {
          "description": "收藏列表"
        }
      }
    }
  },
  "/Batchgetfavitem": {
    "post": {
      "summary": "批量获取收藏详情",
      "description": "根据收藏 ID 批量获取收藏项的完整内容与 XML 详情。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "fav_id": "1,2"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "收藏详情列表"
        }
      }
    }
  },
  "/GetFriendTimeline": {
    "post": {
      "summary": "获取指定好友朋友圈",
      "description": "通过原生 mmsnsuserpage CGI 读取指定好友的服务端相册，支持 id 分页；失败不回退本地缓存。支持已核验的 Weixin 4.1.15.13。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "wxid": "wxid_target",
              "count": 20,
              "id": ""
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "好友朋友圈数据"
        }
      }
    }
  },
  "/mmsnsuserpage": {
    "post": {
      "summary": "获取指定好友朋友圈 (PB 路由名)",
      "description": "GetFriendTimeline 的兼容路由，使用原生网络请求；id 为十进制 uint64 朋友圈 ID 字符串。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "wxid": "wxid_target",
              "count": 20,
              "id": ""
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "好友朋友圈数据"
        }
      }
    }
  },
  "/SendTextMsg_NoSrc": {
    "post": {
      "summary": "发送文本消息_无源",
      "description": "ShowDoc 接口别名：向指定好友、群聊或文件助手发送文本消息（无源）。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "toid": "filehelper",
              "msg": "测试无源消息"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "发送结果"
        }
      }
    }
  },
  "/SendAtMsg_NoSrc": {
    "post": {
      "summary": "发送群 @ 消息",
      "description": "向指定群聊发送文本消息并 @ 指定群成员。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "gid": "25694009164@chatroom",
              "wxidlist": "wxid_sjx2weyau5jh22",
              "msg": "请问物资何时送达？"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "发送结果"
        }
      }
    }
  },
  "/SendImgMsg_NoSrc": {
    "post": {
      "summary": "发送图片消息_无源",
      "description": "向指定目标发送本地图片（无源）。",
      "requestBody": {
        "required": true,
        "content": {
          "application/json": {
            "example": {
              "toid": "filehelper",
              "path": "C:\\test.png"
            }
          }
        }
      },
      "responses": {
        "200": {
          "description": "发送结果"
        }
      }
    }
  },
  "/GetSelfAddress": {
    "post": {
      "summary": "获取本人收货地址信息",
      "description": "获取微信账号中绑定的个人位置与地址信息。",
      "requestBody": {
        "content": {
          "application/json": {
            "example": {}
          }
        }
      },
      "responses": {
        "200": {
          "description": "个人地址"
        }
      }
    }
  },
  "/getonlineinfo": {
    "post": {
      "summary": "获取在线状态",
      "description": "获取当前微信账号的登录在线状态与设备信息。",
      "requestBody": {
        "content": {
          "application/json": {
            "example": {}
          }
        }
      },
      "responses": {
        "200": {
          "description": "在线状态"
        }
      }
    }
  },
  "/GetQRCode": {
    "post": {
      "summary": "获取本人二维码",
      "description": "获取当前登录账号的个人名片二维码链接。",
      "requestBody": {
        "content": {
          "application/json": {
            "example": {}
          }
        }
      },
      "responses": {
        "200": {
          "description": "个人二维码"
        }
      }
    }
  }
};


Object.assign(doc.paths, newPaths);

fs.writeFileSync(openapiPath, JSON.stringify(doc, null, 2), 'utf8');
console.log('Successfully updated OpenAPI specification with', Object.keys(newPaths).length, 'new endpoints.');
