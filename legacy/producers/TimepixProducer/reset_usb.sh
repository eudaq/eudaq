for i in $(ls /sys/bus/pci/drivers/ehci_hcd/|grep :)
   do sudo echo $i >/sys/bus/pci/drivers/ehci_hcd/unbind 
      sudo echo $i >/sys/bus/pci/drivers/ehci_hcd/bind
done
