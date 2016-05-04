Explorer producer for the ALICE ITS Upgrade
============================================

Description
-----------

This producer is for the use with the Explorer chips of the ALICE ITS Upgrade.
Please contact Magnus Mager (CERN) for further information.

Installation
------------

This producer doesn't need additional software. `cmake -DBUILD_explorer=on` is sufficient to including into EUDAQ.

Configuration
-------------

Example configuration files can be found in the `conf` subfolder. They are meant to be used with a Mimosa/NI telescope. `ni_autotrig_explorer_pedestalmeas.conf` is used to do a pedestal measurement. `ni_coins_explorer_wpedestal.conf` is used for beam data using a previously acquired pedestal run. `ni_coins_explorer_wthreshold.conf` is for data taking with a fixed global threshold.
