from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, 
                             QVBoxLayout, QHBoxLayout, QGridLayout, QLabel)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QFont
import sys
import can
import threading

class CanDataSource:
    """Reads vehicle data from CAN bus"""
    def __init__(self, channel='test_channel', interface='virtual'):
        try:
            self.bus = can.interface.Bus(channel=channel, interface=interface)
            print(f"CAN bus connected: {interface} on {channel}")
        except Exception as e:
            print(f"Error connecting to CAN: {e}")
            self.bus = None
        
        # Data storage with default values
        self.motor_rpm = 0
        self.vehicle_speed = 0
        self.coolant_temp = 0.0
        self.lv_battery = 0.0
        self.hv_accumulator = 0.0
        self.soc = 0
        
        # Start listener thread
        self.running = True
        self.thread = threading.Thread(target=self._listen_can, daemon=True)
        self.thread.start()
    
    def _listen_can(self):
        """Background thread that reads CAN messages"""
        if not self.bus:
            return
            
        while self.running:
            msg = self.bus.recv(timeout=0.1)
            if msg:
                self._parse_message(msg)
    
    def _parse_message(self, msg):
        """Decode CAN message and update data"""
        try:
            if msg.arbitration_id == 0x100:  # Motor RPM
                self.motor_rpm = int.from_bytes(msg.data[:2], byteorder='big')
            elif msg.arbitration_id == 0x101:  # Vehicle Speed
                self.vehicle_speed = msg.data[0]
            elif msg.arbitration_id == 0x102:  # Coolant temp (temp * 10)
                self.coolant_temp = int.from_bytes(msg.data[:2], byteorder='big') / 10.0
            elif msg.arbitration_id == 0x103:  # LV Battery (voltage * 100)
                self.lv_battery = int.from_bytes(msg.data[:2], byteorder='big') / 100.0
            elif msg.arbitration_id == 0x104:  # HV Accumulator (voltage * 10)
                self.hv_accumulator = int.from_bytes(msg.data[:2], byteorder='big') / 10.0
            elif msg.arbitration_id == 0x105:  # SoC percentage
                self.soc = msg.data[0]
        except Exception as e:
            print(f"Error parsing CAN message {hex(msg.arbitration_id)}: {e}")
    
    def get_motor_rpm(self):
        return self.motor_rpm
    
    def get_vehicle_speed(self):
        return self.vehicle_speed
    
    def get_coolant_temp(self):
        return self.coolant_temp
    
    def get_battery_voltage(self):
        return self.lv_battery
    
    def get_accumulator_voltage(self):
        return self.hv_accumulator
    
    def get_accumulator_soc(self):
        return self.soc
    
    def stop(self):
        self.running = False
        if self.thread.is_alive():
            self.thread.join()

class FSElectricDashboard(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("FS Electric Dashboard - CAN")
        self.setGeometry(100, 100, 800, 480)
        self.setStyleSheet("background-color: #000000;")
        
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout()
        central_widget.setLayout(main_layout)
        
        # === TOP SECTION: Motor RPM ===
        self.rpm_label = QLabel("0")
        self.rpm_label.setAlignment(Qt.AlignCenter)
        self.rpm_label.setFont(QFont("Arial", 72, QFont.Bold))
        self.rpm_label.setStyleSheet("color: #00FF41;")
        
        rpm_text = QLabel("MOTOR RPM")
        rpm_text.setAlignment(Qt.AlignCenter)
        rpm_text.setFont(QFont("Arial", 18))
        rpm_text.setStyleSheet("color: #AAAAAA;")
        
        main_layout.addWidget(self.rpm_label, 2)
        main_layout.addWidget(rpm_text)
        
        # === MIDDLE SECTION: Speed ===
        speed_layout = QVBoxLayout()
        self.speed_label = QLabel("0")
        self.speed_label.setAlignment(Qt.AlignCenter)
        self.speed_label.setFont(QFont("Arial", 56, QFont.Bold))
        self.speed_label.setStyleSheet("color: #00D4FF;")
        
        speed_text = QLabel("km/h")
        speed_text.setAlignment(Qt.AlignCenter)
        speed_text.setFont(QFont("Arial", 16))
        speed_text.setStyleSheet("color: #AAAAAA;")
        
        speed_layout.addWidget(self.speed_label)
        speed_layout.addWidget(speed_text)
        main_layout.addLayout(speed_layout, 1)
        
        # === BOTTOM SECTION: Telemetry Grid ===
        grid = QGridLayout()
        grid.setSpacing(15)
        
        # HV Accumulator Voltage
        acc_volt_label = QLabel("HV Accumulator")
        acc_volt_label.setStyleSheet("color: #888888; font-size: 13px;")
        acc_volt_label.setAlignment(Qt.AlignCenter)
        self.acc_volt_value = QLabel("0.0 V")
        self.acc_volt_value.setStyleSheet("color: #FF6B00; font-size: 22px; font-weight: bold;")
        self.acc_volt_value.setAlignment(Qt.AlignCenter)
        grid.addWidget(acc_volt_label, 0, 0)
        grid.addWidget(self.acc_volt_value, 1, 0)
        
        # SoC
        acc_soc_label = QLabel("SoC")
        acc_soc_label.setStyleSheet("color: #888888; font-size: 13px;")
        acc_soc_label.setAlignment(Qt.AlignCenter)
        self.acc_soc_value = QLabel("0%")
        self.acc_soc_value.setStyleSheet("color: #00FF41; font-size: 22px; font-weight: bold;")
        self.acc_soc_value.setAlignment(Qt.AlignCenter)
        grid.addWidget(acc_soc_label, 0, 1)
        grid.addWidget(self.acc_soc_value, 1, 1)
        
        # Coolant
        coolant_label = QLabel("Coolant")
        coolant_label.setStyleSheet("color: #888888; font-size: 13px;")
        coolant_label.setAlignment(Qt.AlignCenter)
        self.coolant_value = QLabel("0.0°C")
        self.coolant_value.setStyleSheet("color: #00D4FF; font-size: 22px; font-weight: bold;")
        self.coolant_value.setAlignment(Qt.AlignCenter)
        grid.addWidget(coolant_label, 0, 2)
        grid.addWidget(self.coolant_value, 1, 2)
        
        # LV Battery
        lv_batt_label = QLabel("LV Battery")
        lv_batt_label.setStyleSheet("color: #888888; font-size: 13px;")
        lv_batt_label.setAlignment(Qt.AlignCenter)
        self.lv_batt_value = QLabel("0.0 V")
        self.lv_batt_value.setStyleSheet("color: #FFFFFF; font-size: 22px; font-weight: bold;")
        self.lv_batt_value.setAlignment(Qt.AlignCenter)
        grid.addWidget(lv_batt_label, 0, 3)
        grid.addWidget(self.lv_batt_value, 1, 3)
        
        main_layout.addLayout(grid, 1)
        
        # === DATA SOURCE (CAN) ===
        self.data_source = CanDataSource()
        
        # Timer to update display at 20 Hz
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_display)
        self.timer.start(50)
    
    def update_display(self):
        """Update all display values from CAN data"""
        # Motor RPM with color zones
        motor_rpm = self.data_source.get_motor_rpm()
        self.rpm_label.setText(str(motor_rpm))
        
        if motor_rpm > 13000:
            self.rpm_label.setStyleSheet("color: #FF0000;")
        elif motor_rpm > 11000:
            self.rpm_label.setStyleSheet("color: #FFA500;")
        else:
            self.rpm_label.setStyleSheet("color: #00FF41;")
        
        # Vehicle speed
        self.speed_label.setText(str(self.data_source.get_vehicle_speed()))
        
        # HV Accumulator with warnings
        acc_voltage = self.data_source.get_accumulator_voltage()
        self.acc_volt_value.setText(f"{acc_voltage:.1f} V")
        
        if acc_voltage < 420:
            self.acc_volt_value.setStyleSheet("color: #FF0000; font-size: 22px; font-weight: bold;")
        elif acc_voltage < 480:
            self.acc_volt_value.setStyleSheet("color: #FFA500; font-size: 22px; font-weight: bold;")
        else:
            self.acc_volt_value.setStyleSheet("color: #FF6B00; font-size: 22px; font-weight: bold;")
        
        # SoC with warnings
        soc = self.data_source.get_accumulator_soc()
        self.acc_soc_value.setText(f"{soc}%")
        
        if soc < 20:
            self.acc_soc_value.setStyleSheet("color: #FF0000; font-size: 22px; font-weight: bold;")
        elif soc < 40:
            self.acc_soc_value.setStyleSheet("color: #FFA500; font-size: 22px; font-weight: bold;")
        else:
            self.acc_soc_value.setStyleSheet("color: #00FF41; font-size: 22px; font-weight: bold;")
        
        # Coolant with warnings
        coolant = self.data_source.get_coolant_temp()
        self.coolant_value.setText(f"{coolant:.1f}°C")
        
        if coolant > 75:
            self.coolant_value.setStyleSheet("color: #FF0000; font-size: 22px; font-weight: bold;")
        elif coolant > 65:
            self.coolant_value.setStyleSheet("color: #FFA500; font-size: 22px; font-weight: bold;")
        else:
            self.coolant_value.setStyleSheet("color: #00D4FF; font-size: 22px; font-weight: bold;")
        
        # LV Battery
        lv_voltage = self.data_source.get_battery_voltage()
        self.lv_batt_value.setText(f"{lv_voltage:.2f} V")
        
        if lv_voltage < 11.5:
            self.lv_batt_value.setStyleSheet("color: #FF0000; font-size: 22px; font-weight: bold;")
        else:
            self.lv_batt_value.setStyleSheet("color: #FFFFFF; font-size: 22px; font-weight: bold;")
    
    def closeEvent(self, event):
        """Clean shutdown"""
        self.data_source.stop()
        event.accept()

if __name__ == "__main__":
    app = QApplication(sys.argv)
    dashboard = FSElectricDashboard()
    dashboard.show()
    sys.exit(app.exec_())
