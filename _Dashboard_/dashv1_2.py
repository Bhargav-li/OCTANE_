# Including Both Left and Right RPM

from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QLabel)
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtGui import QFont
import sys
import math
import random

class ElectricFSDataSource:
    """Simulates electric Formula Student vehicle data"""
    def __init__(self):
        self.time = 0
        
    def update(self):
        self.time += 0.05  # 50ms increment
        
    def get_motor_right_rpm(self):
        return int(abs(7500 + 5000 * math.sin(self.time * 0.4)))

    def get_motor_left_rpm(self):
        return int(abs(7500 + 5000 * math.sin(self.time * 0.4 + 1.2)))  # phase offset

    def get_vehicle_speed(self):
        # Speed: 0-120 km/h
        return int(abs(45 + 35 * math.sin(self.time * 0.3)))
    
    def get_coolant_temp(self):
        # Motor/inverter coolant: 30-80°C
        return round(55 + 15 * math.sin(self.time * 0.1) + random.uniform(-1, 1), 1)
    
    def get_battery_voltage(self):
        # LV battery (12V system): 11-14V
        return round(12.8 + 0.6 * math.sin(self.time * 0.15), 2)
    
    def get_accumulator_voltage(self):
        # HV accumulator (typical FS): 400-600V nominal
        # Gradually decreases as it discharges
        base = 550 - (self.time * 0.5)  # Slow discharge
        return round(max(400, base + 10 * math.sin(self.time * 0.2)), 1)
    
    def get_accumulator_soc(self):
        # State of Charge: 0-100%
        # Decreases over time
        soc = 100 - (self.time * 0.3)
        return max(0, int(soc + 5 * math.sin(self.time * 0.25)))

class FSElectricDashboard(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("FS Electric Dashboard")
        self.setGeometry(100, 100, 800, 480)
        self.setStyleSheet("background-color: #000000;")
        
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout()
        central_widget.setLayout(main_layout)
        
        # === TOP SECTION: Motor RPM (Primary display) ===
        rpm_layout = QHBoxLayout()

        # Left Motor
        left_layout = QVBoxLayout()
        self.rpm_left_label = QLabel("0")
        self.rpm_left_label.setAlignment(Qt.AlignCenter)
        self.rpm_left_label.setFont(QFont("Arial", 72, QFont.Bold))
        self.rpm_left_label.setStyleSheet("color: #00FF41;")
        rpm_left_text = QLabel("LEFT MOTOR RPM")
        rpm_left_text.setAlignment(Qt.AlignCenter)
        rpm_left_text.setFont(QFont("Arial", 18))
        rpm_left_text.setStyleSheet("color: #AAAAAA;")
        left_layout.addWidget(self.rpm_left_label)
        left_layout.addWidget(rpm_left_text)
        
        # Right Motor
        right_layout = QVBoxLayout()
        self.rpm_right_label = QLabel("0")
        self.rpm_right_label.setAlignment(Qt.AlignCenter)
        self.rpm_right_label.setFont(QFont("Arial", 72, QFont.Bold))
        self.rpm_right_label.setStyleSheet("color: #00FF41;")
        rpm_right_text = QLabel("RIGHT MOTOR RPM")
        rpm_right_text.setAlignment(Qt.AlignCenter)
        rpm_right_text.setFont(QFont("Arial", 18))
        rpm_right_text.setStyleSheet("color: #AAAAAA;")
        right_layout.addWidget(self.rpm_right_label)
        right_layout.addWidget(rpm_right_text)

        rpm_layout.addLayout(left_layout)
        rpm_layout.addLayout(right_layout)
        main_layout.addLayout(rpm_layout, 2)
        
        # === MIDDLE SECTION: Speed (Secondary priority) ===
        speed_layout = QVBoxLayout()
        self.speed_label = QLabel("0")
        self.speed_label.setAlignment(Qt.AlignCenter)
        self.speed_label.setFont(QFont("Arial", 56, QFont.Bold))
        self.speed_label.setStyleSheet("color: #00D4FF;")  # Cyan
        
        speed_text = QLabel("km/h")
        speed_text.setAlignment(Qt.AlignCenter)
        speed_text.setFont(QFont("Arial", 16))
        speed_text.setStyleSheet("color: #AAAAAA;")
        
        speed_layout.addWidget(self.speed_label)
        speed_layout.addWidget(speed_text)
        main_layout.addLayout(speed_layout, 1)
        
        # === BOTTOM SECTION: Critical telemetry grid ===
        grid = QGridLayout()
        grid.setSpacing(15)
        
        # Column 0: Accumulator Voltage (HV)
        acc_volt_label = QLabel("HV Accumulator")
        acc_volt_label.setStyleSheet("color: #888888; font-size: 13px;")
        acc_volt_label.setAlignment(Qt.AlignCenter)
        self.acc_volt_value = QLabel("0.0 V")
        self.acc_volt_value.setStyleSheet("color: #FF6B00; font-size: 22px; font-weight: bold;")
        self.acc_volt_value.setAlignment(Qt.AlignCenter)
        grid.addWidget(acc_volt_label, 0, 0)
        grid.addWidget(self.acc_volt_value, 1, 0)
        
        # Column 1: Accumulator SoC
        acc_soc_label = QLabel("SoC")
        acc_soc_label.setStyleSheet("color: #888888; font-size: 13px;")
        acc_soc_label.setAlignment(Qt.AlignCenter)
        self.acc_soc_value = QLabel("0%")
        self.acc_soc_value.setStyleSheet("color: #00FF41; font-size: 22px; font-weight: bold;")
        self.acc_soc_value.setAlignment(Qt.AlignCenter)
        grid.addWidget(acc_soc_label, 0, 1)
        grid.addWidget(self.acc_soc_value, 1, 1)
        
        # Column 2: Coolant Temperature
        coolant_label = QLabel("Coolant")
        coolant_label.setStyleSheet("color: #888888; font-size: 13px;")
        coolant_label.setAlignment(Qt.AlignCenter)
        self.coolant_value = QLabel("0.0°C")
        self.coolant_value.setStyleSheet("color: #00D4FF; font-size: 22px; font-weight: bold;")
        self.coolant_value.setAlignment(Qt.AlignCenter)
        grid.addWidget(coolant_label, 0, 2)
        grid.addWidget(self.coolant_value, 1, 2)
        
        # Column 3: LV Battery
        lv_batt_label = QLabel("LV Battery")
        lv_batt_label.setStyleSheet("color: #888888; font-size: 13px;")
        lv_batt_label.setAlignment(Qt.AlignCenter)
        self.lv_batt_value = QLabel("0.0 V")
        self.lv_batt_value.setStyleSheet("color: #FFFFFF; font-size: 22px; font-weight: bold;")
        self.lv_batt_value.setAlignment(Qt.AlignCenter)
        grid.addWidget(lv_batt_label, 0, 3)
        grid.addWidget(self.lv_batt_value, 1, 3)
        
        main_layout.addLayout(grid, 1)
        
        # === DATA SOURCE AND UPDATE TIMER ===
        self.data_source = ElectricFSDataSource()
        
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_display)
        self.timer.start(50)  # 20 Hz refresh
    
    def update_display(self):
        """Update all display values"""
        self.data_source.update()
        
        # Left Motor RPM
        motor_left_rpm = self.data_source.get_motor_left_rpm()
        self.rpm_left_label.setText(str(motor_left_rpm))
        if motor_left_rpm > 13000:
            self.rpm_left_label.setStyleSheet("color: #FF0000;")
        elif motor_left_rpm > 11000:
            self.rpm_left_label.setStyleSheet("color: #FFA500;")
        else:
            self.rpm_left_label.setStyleSheet("color: #00FF41;")

        # Right Motor RPM
        motor_right_rpm = self.data_source.get_motor_right_rpm()
        self.rpm_right_label.setText(str(motor_right_rpm))
        if motor_right_rpm > 13000:
            self.rpm_right_label.setStyleSheet("color: #FF0000;")
        elif motor_right_rpm > 11000:
            self.rpm_right_label.setStyleSheet("color: #FFA500;")
        else:
            self.rpm_right_label.setStyleSheet("color: #00FF41;")
        
        # Vehicle speed
        self.speed_label.setText(str(self.data_source.get_vehicle_speed()))
        
        # Accumulator voltage with color coding
        acc_voltage = self.data_source.get_accumulator_voltage()
        self.acc_volt_value.setText(f"{acc_voltage} V")
        
        if acc_voltage < 420:  # Low voltage warning
            self.acc_volt_value.setStyleSheet("color: #FF0000; font-size: 22px; font-weight: bold;")
        elif acc_voltage < 480:
            self.acc_volt_value.setStyleSheet("color: #FFA500; font-size: 22px; font-weight: bold;")
        else:
            self.acc_volt_value.setStyleSheet("color: #FF6B00; font-size: 22px; font-weight: bold;")
        
        # SoC with color coding
        soc = self.data_source.get_accumulator_soc()
        self.acc_soc_value.setText(f"{soc}%")
        
        if soc < 20:  # Critical
            self.acc_soc_value.setStyleSheet("color: #FF0000; font-size: 22px; font-weight: bold;")
        elif soc < 40:  # Warning
            self.acc_soc_value.setStyleSheet("color: #FFA500; font-size: 22px; font-weight: bold;")
        else:
            self.acc_soc_value.setStyleSheet("color: #00FF41; font-size: 22px; font-weight: bold;")
        
        # Coolant temperature with warning
        coolant = self.data_source.get_coolant_temp()
        self.coolant_value.setText(f"{coolant}°C")
        
        if coolant > 75:  # Overheating
            self.coolant_value.setStyleSheet("color: #FF0000; font-size: 22px; font-weight: bold;")
        elif coolant > 65:  # Getting hot
            self.coolant_value.setStyleSheet("color: #FFA500; font-size: 22px; font-weight: bold;")
        else:
            self.coolant_value.setStyleSheet("color: #00D4FF; font-size: 22px; font-weight: bold;")
        
        # LV Battery
        lv_voltage = self.data_source.get_battery_voltage()
        self.lv_batt_value.setText(f"{lv_voltage} V")
        
        if lv_voltage < 11.5:  # Low LV battery
            self.lv_batt_value.setStyleSheet("color: #FF0000; font-size: 22px; font-weight: bold;")
        else:
            self.lv_batt_value.setStyleSheet("color: #FFFFFF; font-size: 22px; font-weight: bold;")

if __name__ == "__main__":
    app = QApplication(sys.argv)
    dashboard = FSElectricDashboard()
    dashboard.show()
    sys.exit(app.exec_())
