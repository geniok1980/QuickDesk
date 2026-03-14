# QuickDesk 用户注册/登录 API 对接文档

## 1. 接口概览

| 接口 | 方法 | 路径 | 功能 | 权限 |
|------|------|------|------|------|
| 用户注册 | POST | `/api/v1/user/register` | 注册新用户 | 公开 |
| 用户登录 | POST | `/api/v1/user/login` | 用户登录获取 token | 公开 |

---

## 2. 用户注册接口

**请求方式**: `POST`  
**请求路径**: `/api/v1/user/register`  
**Content-Type**: `application/json`

### 请求参数

| 参数名 | 类型 | 必选 | 描述 | 默认值 |
|--------|------|------|------|--------|
| username | string | 是 | 用户名（唯一） | - |
| password | string | 是 | 密码（至少 6 位） | - |
| phone | string | 否 | 手机号 | - |
| email | string | 否 | 邮箱 | - |
| level | string | 否 | 用户等级 V1~V5 | `"V1"` |
| channelType | string | 否 | 通道类型 | `"全球"` |

### 成功响应 (200)

```json
{
  "message": "注册成功",
  "user": {
    "id": 1,
    "username": "testuser",
    "phone": "13800138000",
    "email": "test@example.com",
    "level": "V1",
    "deviceCount": 0,
    "channelType": "全球",
    "status": true,
    "createdAt": "2026-03-08T10:00:00Z",
    "updatedAt": "2026-03-08T10:00:00Z"
  }
}
```

### 失败响应

| HTTP 状态码 | 响应体 | 原因 |
|-------------|--------|------|
| 400 | `{"error":"用户名已存在"}` | 用户名重复 |
| 400 | `{"error":"invalid request"}` | 参数格式错误 |
| 500 | `{"error":"创建用户失败"}` | 数据库错误 |

---

## 3. 用户登录接口

**请求方式**: `POST`  
**请求路径**: `/api/v1/user/login`  
**Content-Type**: `application/json`

### 请求参数

| 参数名 | 类型 | 必选 | 描述 |
|--------|------|------|------|
| username | string | 是 | 用户名 |
| password | string | 是 | 密码 |

### 成功响应 (200)

```json
{
  "token": "5f4dcc3b5aa765d61d8327deb882cf99...",
  "user": {
    "id": 1,
    "username": "testuser",
    "phone": "13800138000",
    "email": "test@example.com",
    "level": "V1",
    "deviceCount": 0,
    "channelType": "全球",
    "status": true,
    "createdAt": "2026-03-08T10:00:00Z",
    "updatedAt": "2026-03-08T10:00:00Z"
  }
}
```

### 失败响应

| HTTP 状态码 | 响应体 | 原因 |
|-------------|--------|------|
| 400 | `{"error":"invalid request"}` | 参数格式错误 |
| 401 | `{"error":"用户名或密码错误"}` | 凭据不正确 |
| 403 | `{"error":"账号已被禁用"}` | 用户已被管理员禁用 |

---

## 4. Token 使用说明

- **Token 有效期**: 7 天（服务重启后失效，in-memory 存储）
- **使用方式**: 请求头添加 `Authorization: Bearer {token}`，或 query 参数 `?token={token}`

---

## 5. 代码示例

### JavaScript (fetch)

```javascript
// 注册
const res = await fetch('/api/v1/user/register', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ username: 'alice', password: 'pass123' })
});
const data = await res.json();

// 登录
const loginRes = await fetch('/api/v1/user/login', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ username: 'alice', password: 'pass123' })
});
const { token, user } = await loginRes.json();

// 认证请求
const profile = await fetch('/api/v1/user/devices', {
  headers: { 'Authorization': `Bearer ${token}` }
});
```

### Python (requests)

```python
import requests

BASE = 'http://localhost:8000'

# 注册
r = requests.post(f'{BASE}/api/v1/user/register',
                  json={'username': 'alice', 'password': 'pass123'})
print(r.json())

# 登录
r = requests.post(f'{BASE}/api/v1/user/login',
                  json={'username': 'alice', 'password': 'pass123'})
token = r.json()['token']

# 认证请求
r = requests.get(f'{BASE}/api/v1/user/devices',
                 headers={'Authorization': f'Bearer {token}'})
print(r.json())
```

---

## 6. 错误码说明

| 错误信息 | HTTP 状态码 | 说明 |
|----------|-------------|------|
| `invalid request` | 400 | 请求参数格式错误 |
| `用户名已存在` | 400 | 用户名已被注册 |
| `用户名或密码错误` | 401 | 凭据不正确 |
| `账号已被禁用` | 403 | 账号被管理员禁用 |
| `密码加密失败` | 500 | 服务器 bcrypt 错误 |
| `创建用户失败` | 500 | 数据库操作失败 |

---

## 7. 注意事项

1. 密码建议至少 6 位
2. 用户名全局唯一，不可重复
3. Token 存储在服务端内存中，服务重启后需重新登录
4. 生产环境请使用 HTTPS
