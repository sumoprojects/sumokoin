# SumokoinWP
A WooCommerce extension for accepting Sumokoin

## Dependancies
This plugin is rather simple but there are a few things that need to be set up before hand.

* A web server! Ideally with the most recent versions of PHP and mysql

* The Sumokoin wallet-cli and Sumokoin wallet-rpc tools found [here](https://sumokoin.org)

* [WordPress](https://wordpress.org)
Wordpress is the backend tool that is needed to use WooCommerce and this Sumokoin plugin

* [WooCommerce](https://woocommerce.com)
This Sumokoin plugin is an extension of WooCommerce, which works with WordPress

## Step 1: Activating the plugin
* Downloading: First of all, you will need to download the plugin. You can download the latest release as a .zip file from https://github.com/pwhelan/sumowp/releases If you wish, you can also download the latest source code from GitHub. This can be done with the command `git clone https://github.com/pwhelan/sumowp.git` or can be downloaded as a zip file from the GitHub web page.

* Unzip the file Sumokoinwp_release.zip if you downloaded the zip from the releases page [here](https://github.com/pwhelan/sumowp/releases).

* Put the plugin in the correct directory: You will need to put the folder named `Sumokoin` from this repo/unzipped release into the wordpress plugins directory. This can be found at `path/to/wordpress/folder/wp-content/plugins`

* Activate the plugin from the WordPress admin panel: Once you login to the admin panel in WordPress, click on "Installed Plugins" under "Plugins". Then simply click "Activate" where it says "Sumokoin - WooCommerce Gateway"

## Step 2: Get a Sumokoin daemon to connect to

### Option 1: Running a full node yourself

To do this: start the Sumokoin daemon on your server and leave it running in the background. This can be accomplished by running `./sumokoind` inside your Sumokoin downloads folder under /Resources/bin. The first time that you start your node, the Sumokoin daemon will download and sync the entire Sumokoin blockchain. This can take several hours and is best done on a machine with at least 4GB of ram, an SSD hard drive (with at least 15GB of free space), and a high speed internet connection.

### Option 2: Connecting to a remote node
[[TODO]]

## Step 3: Setup your Sumokoin wallet-rpc

* Setup a Sumokoin wallet using the sumokoin-wallet-cli tool. If you do not know how to do this you can learn about it at [sumokoin.org](https://sumokoin.org)

* Start the Wallet RPC and leave it running in the background. This can be accomplished by running `./sumokoin-wallet-rpc --rpc-bind-port 18082 --rpc-login username:password --log-level 2 --wallet-file /path/walletfile` where "username:password" is the username and password that you want to use, seperated by a colon and  "/path/walletfile" is your actual wallet file. If you wish to use a remote node you can add the `--daemon-address` flag followed by the address of the node. `--daemon-address node.Sumokoinworld.com:18089` for example.

## Step 4: Setup Sumokoin Gateway in WooCommerce

* Navigate to the "settings" panel in the WooCommerce widget in the WordPress admin panel.

* Click on "Checkout"

* Select "Sumokoin GateWay"

* Check the box labeled "Enable this payment gateway"

* Enter your Sumokoin wallet address in the box labled "Sumokoin Address". If you do not know your address, you can run the `address` commmand in your Sumokoin wallet

* Enter the IP address of your server in the box labeled "Sumokoin wallet rpc Host/IP"

* Enter the port number of the Wallet RPC in the box labeled "Sumokoin wallet rpc port" (will be `18082` if you used the above example).

* Enter the username and password that you want to use in their respective feilds

* Click on "Save changes"

## Info on server authentication
It is reccommended that you specify a username/password with your wallet rpc. This can be done by starting your wallet rpc with `sumokoin-wallet-rpc --rpc-bind-port 18082 --rpc-login username:password --wallet-file /path/walletfile` where "username:password" is the username and password that you want to use, seperated by a colon. Alternatively, you can use the `--restricted-rpc` flag with the wallet rpc like so `./sumokoin-wallet-rpc --testnet --rpc-bind-port 18082 --restricted-rpc --wallet-file wallet/path`.

## Donate to the team
SUMO Address : `Sumoo6aEVZnMNKAyw2aZgE8b3WYmCyBdn1y5eLNh2XRdDUQoBt9gqh9cSdS4jT7Ss8Vbbn5XX5f34LDN43KVyB5X1CqUdysB4R2`
