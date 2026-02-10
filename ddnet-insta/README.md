# Building the Server with Docker

```shell
cd ddnet-insta/
docker buildx build -t insta:latest -f Dockerfile ..

# Run server
docker run -d \
  --name ddnet-server \
  --restart unless-stopped \
  -v "./autoexec.cfg:/tw/data/autoexec.cfg:ro" \
  -v "./reset.cfg:/tw/data/reset.cfg:ro" \
  -p 8303:8303/udp \
  insta:latest
```
Or use [docker-compose.yml](./docker-compose.yml)
