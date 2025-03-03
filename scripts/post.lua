wrk.method = "POST"
wrk.body   = '{"id": 68, "value": "ba"}'
wrk.headers["Content-Type"] = "application/json"

-- Function to handle responses and log errors
response = function(status, headers, body)
    if status ~= 200 then
        print("Error: Status code " .. status)
    end
end