"""
Пакет автоматической калибровки и сквозной верификации зонда «Картограф».

Организация: НПФ «АМК Горизонт»
Разработал: зам начальника ОП Смирнов С. Г.
"""

from .bin_parser import read_cartograf_bin, FrameRecord
from .segmentation import (
    process_frames_interval,
    segment_united_file,
    segments_to_dataframe,
    extract_height_from_filename,
    HeightSegment,
)
from .calibration_core import (
    run_full_calibration,
    calibrate_frequency,
    FullCalResult,
    FreqCalResult,
    ChannelCalResult,
)
from .verifier import (
    verify_full_calibration,
    verify_frequency_geosignal,
    compute_theoretical_geo,
    FreqVerificationReport,
    ChannelGeoVerification,
)
from .fourier_analysis import (
    analyze_sector_data,
    compare_processor_with_raw,
    generate_hardware_and_noise_report,
    generate_fourier_noise_image,
    SectorHarmonics,
    ProcessorComparison,
)
from .metro_writer import save_metrology_bin
from .report import (
    generate_text_report,
    generate_calibration_plots,
    generate_geosignal_table_txt,
    generate_geosignal_verification_image,
)

__all__ = [
    'read_cartograf_bin',
    'FrameRecord',
    'process_frames_interval',
    'segment_united_file',
    'segments_to_dataframe',
    'extract_height_from_filename',
    'HeightSegment',
    'run_full_calibration',
    'calibrate_frequency',
    'FullCalResult',
    'FreqCalResult',
    'ChannelCalResult',
    'verify_full_calibration',
    'verify_frequency_geosignal',
    'compute_theoretical_geo',
    'FreqVerificationReport',
    'ChannelGeoVerification',
    'save_metrology_bin',
    'generate_text_report',
    'generate_calibration_plots',
    'generate_geosignal_table_txt',
    'generate_geosignal_verification_image',
    'analyze_sector_data',
    'compare_processor_with_raw',
    'generate_hardware_and_noise_report',
    'generate_fourier_noise_image',
    'SectorHarmonics',
    'ProcessorComparison',
]
