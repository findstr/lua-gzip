local gzip = require "gzip"

local comp = gzip.deflate("helloworld")
print("gzip", #comp)
local out = gzip.inflate(comp)
print("ugzip", out)

