# deep-pedal-stm32_simracing
pedalera de simulación profesional hecha en casa
# 🏎️ Deep Pedal - Pedalera SimRacing DIY

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![STM32](https://img.shields.io/badge/STM32-F103C6-blue)](https://www.st.com/)
[![Collaboration](https://img.shields.io/badge/Collaboration-Human%2BAI-green)](https://deepseek.com)
![Built in](https://img.shields.io/badge/Built_in-2_days-orange)

Una pedalera de simulación profesional hecha en casa, desarrollada en **2 días** mediante colaboración humano-IA.

## ✨ Características

- ✅ **3 pedales independientes** (gas, freno, embrague)
- ✅ **Calibración automática** (10 segundos al inicio)
- ✅ **USB Plug&Play** (Windows/Linux/Mac)
- ✅ **Sin acoplamiento** entre canales ADC
- ✅ **Código optimizado** para 32KB Flash
- ✅ **Hardware mínimo** (STM32 Blue Pill + 3 potenciómetros)

  ## 🛒 Disponible en Cults3D
[![Cults3D](https://img.shields.io/badge/Cults3D-Download-orange)](https://cults3d.com/tu-enlace)

¿Prefieres tener todo empaquetado? Descarga el pack completo en Cults3D.

## 📸 Vista previa

![Montaje hardware](hardware/fotos/montaje.jpg) <!-- Añade tu foto aquí -->
*Pedalera Deep Pedal montada y funcionando*

## 🛠️ Hardware necesario

| Componente | Cantidad | Notas |
|------------|----------|-------|
| STM32F103C6 (Blue Pill) | 1 | Clon chino funciona perfecto |
| Potenciómetros lineales 10kΩ | 3 | Preferiblemente de carrera larga |
| Cable USB Mini/Micro | 1 | Para alimentación y datos |
| Cables dupont | 10+ | Para conexiones |
| Base/mecánica | 1 | Madera, metal, o impresión 3D |

### 🔌 Conexiones
| Potenciómetro | Pin STM32 | Función |
|---------------|-----------|---------|
| Gas | PA1 | ADC Channel 1 |
| Freno | PA4 | ADC Channel 4 |
| Embrague | PA5 | ADC Channel 5 |
| VCC | 3.3V | Alimentación |
| GND | GND | Tierra común |

## 💻 Firmware

### Requisitos
- STM32CubeIDE 1.10+
- STM32F1 HAL Libraries
- Conocimientos básicos de programación embedded

### Compilación
```bash
# Opción 1: STM32CubeIDE
1. Abre el proyecto en STM32CubeIDE
2. Project → Build All (Ctrl+B)
3. Conecta ST-Link y flashea

# Opción 2: Makefile (si lo añades)
make flash
