from datetime import datetime, timezone

from sqlalchemy import (
    Column, DateTime, Integer, String, Text, UniqueConstraint)
from sqlalchemy.orm import declarative_base

Base = declarative_base()


class DeviceConfig(Base):
    __tablename__ = 'DeviceConfig'

    id = Column(Integer, primary_key=True, autoincrement=True)
    device_id = Column(String(1024), index=True, nullable=False)
    name = Column(String(255), nullable=False)
    config_data = Column(Text, nullable=False)
    timestamp = Column(DateTime, default=datetime.now(timezone.utc),
                       nullable=False)

    __table_args__ = (UniqueConstraint(
        'device_id', 'name', name='uq_device_name'),)
