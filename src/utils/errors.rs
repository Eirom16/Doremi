use thiserror::Error;

#[derive(Debug, Error, Clone)]
pub enum DoremiError {
    #[error("Error de red: {0}")]
    Network(String),

    #[error("Error de autenticación: {0}")]
    Authentication(String),

    #[error("Error de base de datos: {0}")]
    Database(String),

    #[error("Falta la dependencia externa: {0}")]
    ExternalDependency(String),

    #[error("Error interno del sistema: {0}")]
    Internal(String),
}

impl DoremiError {
    pub fn propagate(&self) {
        log::error!("Error propagado a la UI: {:?}", self);
        let msg = match self {
            DoremiError::Network(detail) => {
                format!("Error de red: {detail}. Verifica tu conexión a internet.")
            }
            DoremiError::Authentication(detail) => {
                format!(
                    "Error de sesión/autenticación: {detail}. Por favor, vuelve a iniciar sesión."
                )
            }
            DoremiError::Database(detail) => {
                format!("Error de base de datos local: {detail}.")
            }
            DoremiError::ExternalDependency(detail) => {
                format!("Falta una dependencia del sistema: {detail}. Algunas funciones podrían no estar disponibles.")
            }
            DoremiError::Internal(detail) => {
                format!("Error interno inesperado: {detail}.")
            }
        };
        crate::bridge::bridge::show_notification(&msg, "error");
    }
}

pub fn propagate_error(err: &DoremiError) {
    err.propagate();
}
