# MoneroWP
A WooCommerce extension for accepting Monero

## Dependancies
This plugin is rather simple but there are a few things that need to be set up before hand.

* A web server! Ideally with the most recent versions of PHP and mysql

* The Monero wallet-cli and Monero wallet-rpc tools found [here](https://getmonero.org/downloads/)

* [WordPress](https://wordpress.org)
Wordpress is the backend tool that is needed to use WooCommerce and this Monero plugin

* [WooCommerce](https://woocommerce.com)
This Monero plugin is an extension of WooCommerce, which works with WordPress

## Step 1: Activating the plugin
* Downloading: First of all, you will need to download the plugin. This can be done with the command `git clone https://github.com/monero-integrations/monerowp.git` or can be downloaded as a zip file from the GitHub web page.

* Put the plugin in the correct directory: You will need to put the folder named `monero` from this repo into the wordpress plugins directory. This can be found at `path/to/wordpress/folder/wp-content/plugins`

* Activate the plugin from the WordPress admin panel: Once you login to the admin panel in WordPress, click on "Installed Plugins" under "Plugins". Then simply click "Activate" where it says "Monero - WooCommerce Gateway"

## Step 2: Setup the Monero Wallet RPC

* Setup a monero wallet using the monero-wallet-cli tool. If you do not know how to do this you can learn about it at [getmonero.org](https://getmonero.org/resources/user-guides/monero-wallet-cli.html)

* Start the monero daemon on your server and leave it running in the background. This can be accomplished by running `./monerod` inside your monero downloads folder.

* Start the Wallet RPC and leave it running in the background. This can be accomplished by running `monero-wallet-rpc --rpc-bind-port 18081 --rpc-login username:password --wallet-file /path/walletfile` where "username:password" is the username and password that you want to use, seperated by a colon and  "/path/walletfile" is your actual wallet file.

## Step 3: Setup Monero Gateway in WooCommerce

* Navigate to the "settings" panel in the WooCommerce widget in the WordPress admin panel.

* Click on "Checkout"

* Select "Monero GateWay"

* Check the box labeled "Enable this payment gateway"

* Enter your monero wallet address in the box labled "Monero Address". If you do not know your address, you can run the `address` commmand in your monero wallet

* Enter the IP address of your server in the box labeled "Daemon Host/IP"

* Enter the port number of the Wallet RPC in the box labeled "Daemon PORT" (will be `18081` if you used the above example).

* Enter the username and password that you want to use in their respective feilds

* Click on "Save changes"

## Info on server authentication
It is reccommended that you specify a username/password with your wallet rpc. This can be done by starting your wallet rpc with `monero-wallet-rpc --rpc-bind-port 18081 --rpc-login username:password --wallet-file /path/walletfile` where "username:password" is the username and password that you want to use, seperated by a colon. Alternatively, you can use the `--restricted-rpc` flag with the wallet rpc like so `./monero-wallet-rpc --testnet --rpc-bind-port 18081 --restricted-rpc --wallet-file wallet/path`.
