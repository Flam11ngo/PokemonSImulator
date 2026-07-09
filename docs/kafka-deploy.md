# Kafka 部署指南

目标：在 `hadoop@192.168.88.129` 上部署 Kafka，接收来自 PokemonSimulator 的两类数据。

## 1. SSH 登录

```bash
ssh hadoop@192.168.88.129
```

## 2. 下载 & 安装 Kafka

```bash
cd ~
wget https://dlcdn.apache.org/kafka/3.9.0/kafka_2.13-3.9.0.tgz
tar xzf kafka_2.13-3.9.0.tgz
mv kafka_2.13-3.9.0 kafka
```

Kafka 自带 ZooKeeper（KRaft 模式无需额外安装）。

## 3. 配置 KRaft（无 ZooKeeper）

```bash
cd ~/kafka

# 生成集群 ID
KAFKA_CLUSTER_ID=$(./bin/kafka-storage.sh random-uuid)
echo $KAFKA_CLUSTER_ID

# 格式化存储
./bin/kafka-storage.sh format -t $KAFKA_CLUSTER_ID -c config/kraft/server.properties
```

编辑 `config/kraft/server.properties`：

```properties
listeners=PLAINTEXT://0.0.0.0:9092
advertised.listeners=PLAINTEXT://192.168.88.129:9092
log.dirs=/tmp/kafka-logs
num.partitions=3
offsets.topic.replication.factor=1
```

## 4. 启动 Kafka

```bash
./bin/kafka-server-start.sh -daemon config/kraft/server.properties
```

验证：

```bash
echo "test" | ./bin/kafka-console-producer.sh --topic test --bootstrap-server localhost:9092
./bin/kafka-console-consumer.sh --topic test --from-beginning --bootstrap-server localhost:9092
```

## 5. 创建 Topic

```bash
# 玩家 UI 操作
./bin/kafka-topics.sh --create \
  --topic player.ui.events \
  --bootstrap-server localhost:9092 \
  --partitions 3 \
  --replication-factor 1

# 对战日志
./bin/kafka-topics.sh --create \
  --topic battle.logs \
  --bootstrap-server localhost:9092 \
  --partitions 3 \
  --replication-factor 1
```

验证：

```bash
./bin/kafka-topics.sh --list --bootstrap-server localhost:9092
```

## 6. 网络

确保 Windows 端能访问 9092 端口：

```bash
# 在 hadoop 节点上检查
ss -tlnp | grep 9092

# 如果有防火墙
sudo ufw allow 9092
# 或
sudo firewall-cmd --add-port=9092/tcp --permanent && sudo firewall-cmd --reload
```

在 Windows 上测试连通性：

```powershell
Test-NetConnection 192.168.88.129 -Port 9092
```

## 7. PokemonSimulator 配置

`api-server/config.py` 已配置：

```python
KAFKA_BROKER = "192.168.88.129:9092"

KAFKA_TOPIC_UI_EVENTS = "player.ui.events"
KAFKA_TOPIC_BATTLE_LOG = "battle.logs"
```

## 8. 消费数据验证

```bash
# 实时查看 UI 事件
./bin/kafka-console-consumer.sh --topic player.ui.events --bootstrap-server localhost:9092

# 查看对战日志（含历史）
./bin/kafka-console-consumer.sh --topic battle.logs --from-beginning --bootstrap-server localhost:9092
```

## 9. 开机自启 (systemd)

```bash
sudo tee /etc/systemd/system/kafka.service << 'EOF'
[Unit]
Description=Apache Kafka
After=network.target

[Service]
Type=simple
User=hadoop
WorkingDirectory=/home/hadoop/kafka
ExecStart=/home/hadoop/kafka/bin/kafka-server-start.sh /home/hadoop/kafka/config/kraft/server.properties
ExecStop=/home/hadoop/kafka/bin/kafka-server-stop.sh
Restart=on-failure

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable --now kafka
```

大功告成。启动 PokemonSimulator 后，数据自动推送到 Kafka。
