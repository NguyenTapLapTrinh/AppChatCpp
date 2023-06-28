# AppChatCpp
Console Version 1.0
Chức năng: Có thread cho cả server và client, gửi tin nhắn cả 2 bên không cần đợi bên kia phản hồi.
Biên dịch server: g++ server.cpp -o server -lws2_32
Biên dịch client: g++ client.cpp -o client -lws2_32
Chạy code server: ./server
Chạy code client: ./client + IP (Dò trong ipconfig kiếm IP, Ex: 192.168.1.202)
Lưu ý: GPT chủ yếu, code chưa hiểu hết đâu nên đừng hỏi :v