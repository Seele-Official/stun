import requests
if __name__ == "__main__":
    # Download the list of valid IPv4 addresses
    print("Downloading list of valid IPv4 addresses...")
    response = requests.get('https://raw.githubusercontent.com/pradt2/always-online-stun/master/valid_ipv4s.txt')
    if response.status_code == 200:
        with open('valid_ipv4s.txt', 'wb') as f:
            f.write(response.content)
        print("Download complete.")
    else:
        print("Failed to download the list of valid IPv4 addresses.")
        exit(1)