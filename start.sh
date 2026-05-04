#!/bin/bash

#kill and restart whatever is using my port
sudo fuser -k 10000/tcp

# restart nginx
sudo systemctl restart nginx

echo "Starting nginx using custom config"
sudo nginx -c "$(pwd)/my-nginx/nginx.conf"
