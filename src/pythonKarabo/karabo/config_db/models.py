from datetime import datetime, timezone

from sqlmodel import Field, Relationship, SQLModel, UniqueConstraint


class NamedDeviceInstance(SQLModel, table=True):
    __tablename__ = "NamedDeviceInstance"

    id: int | None = Field(default=None, primary_key=True)
    device_id: str = Field(unique=True, index=True)
    class_id: str = Field(max_length=128, nullable=False)
    server_id: str = Field(max_length=128, nullable=False)

    # One-to-many relationship with NamedDeviceConfig
    configurations: list["NamedDeviceConfig"] = Relationship(
        back_populates="instance", sa_relationship_kwargs={
            "cascade": "all, delete-orphan"})


class NamedDeviceConfig(SQLModel, table=True):
    __tablename__ = "NamedDeviceConfig"

    __table_args__ = (UniqueConstraint(
        "device_id", "name", name="uq_device_name"),)

    id: int | None = Field(default=None, primary_key=True)
    device_id: str = Field(
        foreign_key="NamedDeviceInstance.device_id", index=True)
    name: str
    config_data: str
    date: datetime = Field(
        default_factory=lambda: datetime.now(timezone.utc))

    last_loaded: datetime | None = Field(
        default=None, nullable=True)

    # Relationship back to DeviceInstance
    instance: NamedDeviceInstance | None = Relationship(
        back_populates="configurations")
