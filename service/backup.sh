elasticdump \
  --input=http://localhost:9200/supervisor \
  --output=./test_backup/analyzer.json \
  --type=analyzer
elasticdump \
  --input=http://localhost:9200/supervisor \
  --output=./test_backup/mapping.json \
  --type=mapping
elasticdump \
  --input=http://localhost:9200/supervisor \
  --output=./test_backup/data.json \
  --type=data
