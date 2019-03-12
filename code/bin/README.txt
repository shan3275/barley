使用说明：
	A.把pcie_control_reset.sh 和recovery.sh放在驱动加载的目录，例如：frc_tcp_ssn_V2.04_736目录中
	B.在加载驱动后,运行recovery.sh(也就是在运行frc_load.sh脚本之后，运行recovery.sh脚本)
	运行方法 ./recovery.sh  PATH(驱动加载脚本的目录)  scriptNAME(驱动脚本) >/dev/null &(后台运行)
		eg. ./recovery.sh /root/frc_tcp_ssn_V2.04_736/ frc_load.sh >/dev/null &







