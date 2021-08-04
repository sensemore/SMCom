import SMCom

s = SMCom.SMCom(1024, 0, 10)

x = SMCom.pySMCOM_PUBLIC()

event = SMCom.SMCom_event_types(1)
status = SMCom.SMCom_Status_t(1)

class temp(SMCom.SMCom):
    def rx_event_handler_callback(event, status, x):
        print("worked")

s.py_rx_callback(event, status, x)